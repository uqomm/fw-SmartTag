#!/usr/bin/env python3
"""
Gateway Control GUI - STM32 FSK Scanner Interface
Provides a graphical interface to control the gateway via UART

Features:
- Send UART commands
- Start/Stop FSK scanner
- Real-time log viewing
- Save logs to files
- Configure FSK parameters
- View statistics

Author: Engineering Team
Date: October 2025
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
import serial
import serial.tools.list_ports
import threading
import queue
import time
from datetime import datetime
import struct  # For binary data packing
import os


class GatewayControlGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 Gateway Control - FSK Scanner Interface")
        self.root.geometry("1200x800")
        
        # Serial connections
        self.serial_port = None  # For commands and logs (UART1)
        self.connected = False
        self.read_thread = None
        self.stop_thread = False
        
        # Message queue for thread-safe GUI updates
        self.message_queue = queue.Queue()
        
        # Log storage
        self.log_buffer = []
        self.max_log_lines = 10000
        
        # Downloaded logs from device
        self.downloading_saved_logs = False
        self.saved_logs_buffer = []
        self.last_log_time = None
        self.download_timeout_timer = None
        
        # Statistics
        self.stats = {
            'detections': 0,
            'configs_tested': 0,
            'last_detection_time': None
        }
        
        # Failed combinations tracking
        self.failed_combinations = []
        self.current_test_config = {}
        self.test_had_detection = False
        
        # Auto-scroll for log display
        self.auto_scroll_var = tk.BooleanVar(value=True)
        
        # Create GUI
        self.create_gui()
        
        # Start queue processor
        self.process_queue()
        
    def create_gui(self):
        """Create the main GUI layout"""
        
        # ============================================================
        # TOP FRAME - Connection Controls
        # ============================================================
        top_frame = ttk.Frame(self.root, padding="10")
        top_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E))
        
        # Port selection
        ttk.Label(top_frame, text="COM Port:").grid(row=0, column=0, padx=5, sticky='w')
        self.port_combo = ttk.Combobox(top_frame, width=15, state='readonly')
        self.port_combo.grid(row=0, column=1, padx=5)
        
        # Baudrate
        ttk.Label(top_frame, text="Baudrate:").grid(row=0, column=2, padx=5)
        self.baudrate_combo = ttk.Combobox(top_frame, width=10, state='readonly',
                                           values=['9600', '19200', '38400', '57600', '115200', '230400'])
        self.baudrate_combo.set('115200')
        self.baudrate_combo.grid(row=0, column=3, padx=5)
        
        # Connect button
        self.connect_btn = ttk.Button(top_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=4, padx=5)
        
        # Refresh ports button
        ttk.Button(top_frame, text="Refresh", command=self.refresh_ports).grid(row=0, column=5, padx=5)
        
        # Connection status
        self.status_label = ttk.Label(top_frame, text="● Disconnected", foreground="red")
        self.status_label.grid(row=0, column=6, padx=20)
        
        # ============================================================
        # MIDDLE FRAME - Main Content (2 columns)
        # ============================================================
        
        # LEFT COLUMN - Controls
        left_frame = ttk.Frame(self.root, padding="10")
        left_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Scanner Controls
        scanner_group = ttk.LabelFrame(left_frame, text="FSK Scanner Control", padding="10")
        scanner_group.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Button(scanner_group, text="▶ Start Scanner", 
                   command=lambda: self.send_command("START_SCAN")).grid(row=0, column=0, padx=5, pady=5, sticky=(tk.W, tk.E))
        ttk.Button(scanner_group, text="■ Stop Scanner", 
                   command=lambda: self.send_command("STOP_SCAN")).grid(row=0, column=1, padx=5, pady=5, sticky=(tk.W, tk.E))
        ttk.Button(scanner_group, text="↻ Reset Scanner", 
                   command=lambda: self.send_command("RESET_SCAN")).grid(row=1, column=0, columnspan=2, padx=5, pady=5, sticky=(tk.W, tk.E))
        
        # Becker Scanner Controls
        becker_group = ttk.LabelFrame(left_frame, text="Becker Varis Scanner", padding="10")
        becker_group.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Button(becker_group, text="🎯 Smart Becker Scan", 
                   command=lambda: self.send_command("START_SMART_BECKER_SCAN")).grid(row=0, column=0, padx=5, pady=5, sticky=(tk.W, tk.E))
        ttk.Button(becker_group, text="🔍 Full Becker Scan", 
                   command=lambda: self.send_command("START_BECKER_SCAN")).grid(row=0, column=1, padx=5, pady=5, sticky=(tk.W, tk.E))
        
        # Quick Commands
        commands_group = ttk.LabelFrame(left_frame, text="Quick Commands", padding="10")
        commands_group.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)
        
        quick_commands = [
            ("Get Version", "GET_VERSION"),
            ("Get Config", "GET_CONFIG"),
            ("Save Config", "SAVE_CONFIG"),
            ("Get Stats", "GET_STATS"),
            ("Get RSSI", "GET_RSSI"),
            ("Reset Device", "RESET_DEVICE")
        ]
        
        for idx, (label, cmd) in enumerate(quick_commands):
            row = idx // 2
            col = idx % 2
            ttk.Button(commands_group, text=label, 
                       command=lambda c=cmd: self.send_command(c)).grid(
                           row=row, column=col, padx=5, pady=3, sticky=(tk.W, tk.E))
        
        # Custom Command
        custom_group = ttk.LabelFrame(left_frame, text="Custom Command", padding="10")
        custom_group.grid(row=3, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(custom_group, text="Command:").grid(row=0, column=0, padx=5)
        self.custom_cmd_entry = ttk.Entry(custom_group, width=20)
        self.custom_cmd_entry.grid(row=0, column=1, padx=5)
        self.custom_cmd_entry.bind('<Return>', lambda e: self.send_custom_command())
        
        ttk.Button(custom_group, text="Send", 
                   command=self.send_custom_command).grid(row=0, column=2, padx=5)
        
        # FSK Configuration
        config_group = ttk.LabelFrame(left_frame, text="FSK Configuration", padding="10")
        config_group.grid(row=4, column=0, sticky=(tk.W, tk.E), pady=5)
        
        # Frequency
        ttk.Label(config_group, text="Frequency (MHz):").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.freq_entry = ttk.Entry(config_group, width=15)
        self.freq_entry.insert(0, "175.0")
        self.freq_entry.grid(row=0, column=1, padx=5, pady=2)
        
        # Bitrate
        ttk.Label(config_group, text="Bitrate (bps):").grid(row=1, column=0, sticky=tk.W, padx=5)
        self.bitrate_combo = ttk.Combobox(config_group, width=12, state='readonly',
                                          values=['1000', '1200', '2400', '3000', '4800', '6000', 
                                                  '9600', '12000', '19200', '38400'])
        self.bitrate_combo.set('4800')
        self.bitrate_combo.grid(row=1, column=1, padx=5, pady=2)
        
        # Deviation
        ttk.Label(config_group, text="Deviation (Hz):").grid(row=2, column=0, sticky=tk.W, padx=5)
        self.deviation_combo = ttk.Combobox(config_group, width=12, state='readonly',
                                            values=['2500', '3000', '5000', '7500', '10000', 
                                                    '15000', '20000', '25000'])
        self.deviation_combo.set('5000')
        self.deviation_combo.grid(row=2, column=1, padx=5, pady=2)
        
        # Scan timeout
        ttk.Label(config_group, text="Scan Timeout (ms):").grid(row=3, column=0, sticky=tk.W, padx=5)
        self.timeout_entry = ttk.Entry(config_group, width=15)
        self.timeout_entry.insert(0, "1000")
        self.timeout_entry.grid(row=3, column=1, padx=5, pady=2)
        
        ttk.Button(config_group, text="Apply Configuration", 
                   command=self.apply_fsk_config).grid(row=4, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        # Statistics
        stats_group = ttk.LabelFrame(left_frame, text="Statistics", padding="10")
        stats_group.grid(row=5, column=0, sticky=(tk.W, tk.E), pady=5)
        
        self.stats_text = tk.Text(stats_group, height=8, width=35, state='disabled')
        self.stats_text.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        ttk.Button(stats_group, text="Refresh Stats", 
                   command=self.update_statistics).grid(row=1, column=0, pady=5, sticky=(tk.W, tk.E))
        
        # Add auto-refresh for statistics during scanning
        self.auto_refresh_stats = tk.BooleanVar(value=True)
        ttk.Checkbutton(stats_group, text="Auto-refresh stats", 
                        variable=self.auto_refresh_stats).grid(row=3, column=0, pady=5, sticky=(tk.W, tk.E))
        
        # Add progress indicator
        self.scan_progress_var = tk.StringVar(value="Scanner: Stopped")
        ttk.Label(stats_group, textvariable=self.scan_progress_var, 
                 font=('Arial', 9, 'bold')).grid(row=4, column=0, pady=5, sticky=(tk.W, tk.E))
        
        # RIGHT COLUMN - Log Display
        right_frame = ttk.Frame(self.root, padding="10")
        right_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Log controls
        log_control_frame = ttk.Frame(right_frame)
        log_control_frame.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        ttk.Label(log_control_frame, text="Serial Log:", font=('Arial', 10, 'bold')).grid(row=0, column=0, sticky=tk.W)
        
        ttk.Button(log_control_frame, text="💾 Save Log", 
                   command=self.save_log).grid(row=0, column=1, padx=5)
        ttk.Button(log_control_frame, text="📥 Download Saved Logs", 
                   command=self.download_saved_logs).grid(row=0, column=2, padx=5)
        ttk.Button(log_control_frame, text="🔗 Save Combined Logs", 
                   command=self.save_combined_logs).grid(row=0, column=3, padx=5)
        ttk.Button(log_control_frame, text="🗑 Clear Log", 
                   command=self.clear_log).grid(row=0, column=4, padx=5)
        
        # Second row of buttons
        ttk.Button(log_control_frame, text="🗑 Clear Downloaded", 
                   command=self.clear_downloaded_logs).grid(row=1, column=1, padx=5, pady=2)
        
        # Auto-scroll checkbox
        ttk.Checkbutton(log_control_frame, text="Auto-scroll", 
                        variable=self.auto_scroll_var).grid(row=1, column=3, padx=5)
        
        # Auto-query scanner status when connected
        self.auto_query_status = tk.BooleanVar(value=False)
        ttk.Checkbutton(log_control_frame, text="Auto-query status", 
                        variable=self.auto_query_status, 
                        command=self.on_auto_query_toggle).grid(row=1, column=4, padx=5)
        
        # Status query timer
        self.status_query_timer = None
        
        # Log display
        self.log_display = scrolledtext.ScrolledText(right_frame, width=80, height=35, 
                                                      font=('Consolas', 9), state='disabled')
        self.log_display.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        # Configure text tags for colored output
        self.log_display.tag_config('INFO', foreground='green')
        self.log_display.tag_config('WARN', foreground='orange')
        self.log_display.tag_config('ERROR', foreground='red')
        self.log_display.tag_config('DEBUG', foreground='gray')
        self.log_display.tag_config('DETECT', foreground='blue', font=('Consolas', 9, 'bold'))
        
        # ============================================================
        # BOTTOM FRAME - Status Bar
        # ============================================================
        bottom_frame = ttk.Frame(self.root, padding="5")
        bottom_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E))
        
        self.status_bar = ttk.Label(bottom_frame, text="Ready", relief=tk.SUNKEN)
        self.status_bar.pack(fill=tk.X)
        
        # Refresh ports after status bar is created
        self.refresh_ports()
        
        # Configure grid weights for resizing
        self.root.columnconfigure(1, weight=1)
        self.root.rowconfigure(1, weight=1)
        right_frame.columnconfigure(0, weight=1)
        right_frame.rowconfigure(1, weight=1)
        
    def refresh_ports(self):
        """Refresh available COM ports"""
        try:
            ports = [port.device for port in serial.tools.list_ports.comports()]
            if not ports:
                ports = ["No ports found"]
            
            self.port_combo['values'] = ports
            if ports and ports[0] != "No ports found":
                self.port_combo.set(ports[0])
                
        except Exception as e:
            self.port_combo['values'] = [f"Error: {str(e)}"]
            self.update_status_bar(f"Port refresh error: {str(e)}")
        
    def start_status_queries(self):
        """Start periodic status queries"""
        if self.status_query_timer:
            self.root.after_cancel(self.status_query_timer)
        
        def query_status():
            if self.connected and self.auto_query_status.get():
                try:
                    # Send status query
                    frame = self.build_binary_command(0x72)  # QUERY_SCAN_STATUS
                    self.serial_port.write(frame)
                    self.append_log(f">>> AUTO QUERY: CMD=0x72, Frame={frame.hex()}\n", 'DEBUG')
                except:
                    pass  # Ignore errors during auto-query
            
            # Schedule next query in 5 seconds
            if self.connected and self.auto_query_status.get():
                self.status_query_timer = self.root.after(5000, query_status)
        
        # Start immediately
        query_status()
    
    def stop_status_queries(self):
        """Stop periodic status queries"""
        if self.status_query_timer:
            self.root.after_cancel(self.status_query_timer)
            self.status_query_timer = None
    
    def on_auto_query_toggle(self):
        """Handle auto-query checkbox toggle"""
        if self.connected:
            if self.auto_query_status.get():
                self.start_status_queries()
            else:
                self.stop_status_queries()
    
    def build_binary_command(self, command_id, data=b'', module_function=0x00, module_id=0x00):
        """Build a binary command frame using the protocol format (based on FrameBuilder pattern)"""
        # Protocol constants
        START_MARK = 0x7E
        END_MARK = 0x7F
        
        # Frame structure: START(1) + MODULE_FUNCTION(1) + MODULE_ID(1) + CMD(1) + DATA_LEN(1) + DATA + CRC(2) + END(1)
        frame = bytearray([START_MARK, module_function, module_id, command_id, 0, len(data)])
        frame.extend(data)
        frame += self._calculate_crc(frame[1:])
        frame.append(END_MARK)
        
        return bytes(frame)
    
    def build(self, module_function: int, module_id: int, command_id: int, data=b''):
        """Build a binary command frame using FrameBuilder pattern (main build method)"""
        return self.build_binary_command(command_id, data, module_function, module_id)
    
    def _calculate_crc(self, data: bytes) -> bytes:
        """Calculate CRC16 XMODEM checksum (from FrameBuilder)"""
        crc = 0xFFFF
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 0x0001:
                    crc = (crc >> 1) ^ 0xA001
                else:
                    crc >>= 1
        return struct.pack("<H", crc)
        
    def toggle_connection(self):
        """Connect/Disconnect from serial port"""
        if not self.connected:
            self.connect()
        else:
            self.disconnect()
            
    def connect(self):
        """Connect to serial port"""
        try:
            if self.serial_port is None:
                port = self.port_combo.get()
                if port and port != "No ports found":
                    self.serial_port = serial.Serial(
                        port=port,
                        baudrate=115200,
                        bytesize=serial.EIGHTBITS,
                        parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE,
                        timeout=1
                    )
                    self.update_status_bar(f"Connected to {port}")
                    
                    # Start reading thread
                    self.stop_thread = False
                    self.read_thread = threading.Thread(target=self.read_serial, daemon=True)
                    self.read_thread.start()
                    
                else:
                    self.update_status_bar("No valid port selected")
                    return
            
            self.connected = True
            self.connect_btn.config(text="Disconnect")
            self.status_label.config(text="● Connected", foreground="green")
            
            # Start status query timer if enabled
            if self.auto_query_status.get():
                self.start_status_queries()
            
        except Exception as e:
            messagebox.showerror("Connection Error", f"Failed to connect:\n{str(e)}")
            self.update_status_bar(f"Connection failed: {str(e)}")
            self.disconnect()
            
    def disconnect(self):
        """Disconnect from serial port"""
        if self.serial_port:
            self.stop_thread = True
            if self.read_thread:
                self.read_thread.join(timeout=1)
            
            self.serial_port.close()
            self.serial_port = None
            
        self.connected = False
        self.connect_btn.config(text="Connect")
        self.status_label.config(text="● Disconnected", foreground="red")
        self.update_status_bar("Disconnected")
        self.append_log("=== Disconnected ===\n", 'WARN')
        
        # Stop status query timer
        self.stop_status_queries()
        
    def read_serial(self):
        """Background thread to read serial data"""
        while not self.stop_thread and self.serial_port:
            try:
                if self.serial_port.in_waiting:
                    data = self.serial_port.readline()
                    try:
                        text = data.decode('utf-8', errors='ignore').strip()
                        if text:
                            self.message_queue.put(('log', text))
                    except:
                        pass
                time.sleep(0.01)
            except Exception as e:
                self.message_queue.put(('error', f"Read error: {str(e)}"))
                break
                
    def send_command(self, command):
        """Send a command to the gateway using binary protocol"""
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return
        
        try:
            # Map command names to binary command IDs
            command_map = {
                'START_SCAN': 0x70,      # START_FSK_SCAN
                'STOP_SCAN': 0x71,       # STOP_FSK_SCAN
                'RESET_SCAN': 0x74,      # RESET_FSK_SCAN
                'START_BECKER_SCAN': 0x78,    # START_BECKER_SCAN
                'START_SMART_BECKER_SCAN': 0x7B,  # START_SMART_BECKER_SCAN
                'GET_VERSION': 0x10,     # QUERY_MODULE_ID
                'GET_CONFIG': 0x11,      # QUERY_STATUS
                'SAVE_CONFIG': 0x73,     # SAVE_FSK_CONFIG
                'GET_STATS': 0x72,       # QUERY_SCAN_STATUS
                'GET_RSSI': 0x11,        # QUERY_STATUS (temporary mapping)
                'RESET_DEVICE': 0x11,    # QUERY_STATUS (temporary mapping)
                'DUMP_SAVED_LOGS': 0x7C  # DUMP_SAVED_LOGS
            }
            
            cmd_id = command_map.get(command)
            if cmd_id is None:
                messagebox.showerror("Unknown Command", f"Unknown command: {command}")
                return
            
            # Build binary frame
            frame = self.build_binary_command(cmd_id)
            
            # Send the frame
            self.serial_port.write(frame)
            self.append_log(f">>> SENT BINARY: CMD=0x{cmd_id:02X}, Frame={frame.hex()}\n", 'DEBUG')
            
            # Provide better status feedback
            command_names = {
                'START_SCAN': 'Starting FSK Scanner',
                'STOP_SCAN': 'Stopping FSK Scanner', 
                'RESET_SCAN': 'Resetting FSK Scanner',
                'START_BECKER_SCAN': 'Starting Full Becker Scanner',
                'START_SMART_BECKER_SCAN': 'Starting Smart Becker Scanner (10 prioritized configs)',
                'GET_VERSION': 'Querying Version',
                'GET_CONFIG': 'Querying Configuration',
                'SAVE_CONFIG': 'Saving Configuration',
                'GET_STATS': 'Querying Statistics',
                'GET_RSSI': 'Querying RSSI',
                'RESET_DEVICE': 'Resetting Device',
                'DUMP_SAVED_LOGS': 'Downloading Saved Logs from Device'
            }
            status_msg = command_names.get(command, f"Sent {command}")
            self.update_status_bar(status_msg)
            
        except Exception as e:
            messagebox.showerror("Send Error", f"Failed to send command:\n{str(e)}")
            
    def send_custom_command(self):
        """Send custom command - accepts hex command ID optionally followed by hex data"""
        cmd = self.custom_cmd_entry.get().strip()
        if not cmd:
            return
        
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return
        
        try:
            # Parse command: "0x70" or "0x70 deadbeef" (command ID + optional hex data)
            parts = cmd.split()
            if len(parts) >= 1:
                cmd_id_str = parts[0]
                data_hex = parts[1] if len(parts) > 1 else ""
                
                # Parse command ID
                if cmd_id_str.startswith('0x') or cmd_id_str.startswith('0X'):
                    cmd_id = int(cmd_id_str, 16)
                else:
                    cmd_id = int(cmd_id_str, 16)  # Assume hex even without 0x
                
                # Parse optional data
                data = b''
                if data_hex:
                    data = bytes.fromhex(data_hex)
                
                # Build and send binary frame
                frame = self.build_binary_command(cmd_id, data)
                self.serial_port.write(frame)
                self.append_log(f">>> SENT BINARY: CMD=0x{cmd_id:02X}, Data={data.hex() if data else 'None'}, Frame={frame.hex()}\n", 'DEBUG')
                self.custom_cmd_entry.delete(0, tk.END)
                self.update_status_bar(f"Sent custom command: 0x{cmd_id:02X}")
            else:
                messagebox.showerror("Invalid Format", "Use format: CMD_ID [DATA]\nExample: 70 or D1 0000803F")
                
        except ValueError as e:
            messagebox.showerror("Parse Error", f"Invalid hex format:\n{str(e)}\nUse format: CMD_ID [DATA]\nExample: 70 or D1 0000803F")
        except Exception as e:
            messagebox.showerror("Send Error", f"Failed to send:\n{str(e)}")
            
    def apply_fsk_config(self):
        """Apply FSK configuration using binary protocol"""
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return
        
        try:
            freq = float(self.freq_entry.get())
            bitrate = int(self.bitrate_combo.get())
            deviation = int(self.deviation_combo.get())
            timeout = int(self.timeout_entry.get())
            
            # Send configuration commands with binary protocol
            configs = [
                (0xD1, struct.pack('<f', freq)),        # SET_FSK_FREQ - frequency as float
                (0xD2, struct.pack('<I', bitrate)),     # SET_FSK_BITRATE - bitrate as uint32
                (0xD3, struct.pack('<I', deviation)),   # SET_FSK_DEVIATION - deviation as uint32
                (0x75, struct.pack('<I', timeout))      # SET_SCAN_TIMEOUT - timeout as uint32
            ]
            
            for cmd_id, data in configs:
                frame = self.build_binary_command(cmd_id, data)
                self.serial_port.write(frame)
                time.sleep(0.05)  # Small delay between commands
                self.append_log(f">>> SENT BINARY: CMD=0x{cmd_id:02X}, Data={data.hex()}, Frame={frame.hex()}\n", 'DEBUG')
            
            self.update_status_bar("FSK configuration applied")
            messagebox.showinfo("Success", "Configuration applied successfully")
            
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter valid numeric values")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to apply config:\n{str(e)}")
            
    def append_log(self, text, tag='INFO'):
        """Append text to log display"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        formatted_text = f"[{timestamp}] {text}\n"
        
        # Check if this is a saved log from the device (starts with device timestamp)
        is_device_log = text.startswith("[") and "]" in text and len(text.split("]")[0]) >= 10
        
        if self.downloading_saved_logs and is_device_log:
            # This is a saved log from the device
            self.saved_logs_buffer.append(text + "\n")
            self.last_log_time = datetime.now()
            
            # Reset the timeout timer
            if self.download_timeout_timer:
                self.root.after_cancel(self.download_timeout_timer)
            self.download_timeout_timer = self.root.after(2000, self.stop_downloading_logs)  # 2 second timeout
            
            # Also show in main log with different tag
            self.log_display.config(state='normal')
            self.log_display.insert(tk.END, f"[DEVICE] {text}\n", 'DEBUG')
            if self.auto_scroll_var.get():
                self.log_display.see(tk.END)
            self.log_display.config(state='disabled')
            return
        
        # Store in buffer
        self.log_buffer.append(formatted_text)
        if len(self.log_buffer) > self.max_log_lines:
            self.log_buffer.pop(0)
        
        # Update GUI
        self.log_display.config(state='normal')
        self.log_display.insert(tk.END, formatted_text, tag)
        
        if self.auto_scroll_var.get():
            self.log_display.see(tk.END)
        
        self.log_display.config(state='disabled')
        
        # Parse for statistics
        self.parse_log_for_stats(text)
        
        # Auto-refresh statistics if enabled and scanning detected
        if self.auto_refresh_stats.get() and "Test #" in text:
            self.root.after(100, self.update_statistics)
        
    def parse_log_for_stats(self, text):
        """Parse log text for statistics"""
        if "SIGNAL DETECTED" in text:
            self.stats['detections'] += 1
            self.stats['last_detection_time'] = datetime.now()
            self.test_had_detection = True
        
        if "Test #" in text:
            # Check if previous test failed
            if self.current_test_config and not self.test_had_detection and len(self.current_test_config) > 0:
                self.failed_combinations.append(self.current_test_config)
                failed_str = f"FAILED: Freq={self.current_test_config.get('frequency', 'N/A')}, Bitrate={self.current_test_config.get('bitrate', 'N/A')}, Deviation={self.current_test_config.get('deviation', 'N/A')}"
                self.append_log(failed_str + "\n", 'ERROR')
            
            # Start new test
            self.current_test_config = {}
            self.test_had_detection = False
            
            try:
                # Extract config number
                parts = text.split("Test #")[1].split("/")
                if len(parts) >= 2:
                    self.stats['configs_tested'] = int(parts[0])
            except:
                pass
        
        # Parse test parameters
        if "Frequency:" in text or "Scan Frequency:" in text:
            parts = text.split(":", 1)
            if len(parts) >= 2:
                # Extract frequency value, handling both formats
                freq_text = parts[1].strip()
                # Remove "MHz" and other text to get just the frequency
                freq_value = freq_text.split()[0] if freq_text.split() else freq_text
                self.current_test_config['frequency'] = freq_value
        elif "Bitrate:" in text:
            parts = text.split(":", 1)
            if len(parts) >= 2:
                self.current_test_config['bitrate'] = parts[1].strip()
        elif "Deviation:" in text:
            parts = text.split(":", 1)
            if len(parts) >= 2:
                self.current_test_config['deviation'] = parts[1].strip()
        elif "RX Bandwidth:" in text:
            parts = text.split(":", 1)
            if len(parts) >= 2:
                self.current_test_config['rx_bandwidth'] = parts[1].strip()
        elif "Preamble:" in text:
            parts = text.split(":", 1)
            if len(parts) >= 2:
                self.current_test_config['preamble'] = parts[1].strip()
        elif "Sync Word:" in text:
            parts = text.split(":", 1)
            if len(parts) >= 2:
                self.current_test_config['sync_word'] = parts[1].strip()
                
    def update_statistics(self):
        """Update statistics display"""
        stats_text = f"""
Detections: {self.stats['detections']}
Configs Tested: {self.stats['configs_tested']}
Failed Combinations: {len(self.failed_combinations)}

Last Detection:
{self.stats['last_detection_time'].strftime('%Y-%m-%d %H:%M:%S') if self.stats['last_detection_time'] else 'None'}

Log Lines: {len(self.log_buffer)}
        """
        
        self.stats_text.config(state='normal')
        self.stats_text.delete(1.0, tk.END)
        self.stats_text.insert(1.0, stats_text.strip())
        self.stats_text.config(state='disabled')
        
        # Update progress indicator based on recent activity
        if self.stats['configs_tested'] > 0:
            success_rate = (self.stats['detections'] / self.stats['configs_tested']) * 100
            self.scan_progress_var.set(f"Scanner: Active ({self.stats['configs_tested']} tests, {success_rate:.1f}% success)")
        else:
            self.scan_progress_var.set("Scanner: Stopped")
        
    def save_failed_combinations(self):
        """Save failed combinations to file"""
        if not self.failed_combinations:
            messagebox.showinfo("No Failed Combinations", "No failed combinations to save")
            return
        
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("CSV files", "*.csv"), ("All files", "*.*")],
            initialfile=f"failed_combinations_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write("Failed FSK Combinations:\n")
                    f.write("=" * 50 + "\n")
                    for combo in self.failed_combinations:
                        f.write(f"Frequency: {combo.get('frequency', 'N/A')}\n")
                        f.write(f"Bitrate: {combo.get('bitrate', 'N/A')}\n")
                        f.write(f"Deviation: {combo.get('deviation', 'N/A')}\n")
                        f.write(f"RX Bandwidth: {combo.get('rx_bandwidth', 'N/A')}\n")
                        f.write(f"Preamble: {combo.get('preamble', 'N/A')}\n")
                        f.write(f"Sync Word: {combo.get('sync_word', 'N/A')}\n")
                        f.write("-" * 30 + "\n")
                messagebox.showinfo("Success", f"Failed combinations saved to:\n{filename}")
                self.update_status_bar(f"Failed combos saved to {os.path.basename(filename)}")
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save:\n{str(e)}")
                
    def save_log(self):
        """Save log to file"""
        if not self.log_buffer:
            messagebox.showinfo("Empty Log", "No log data to save")
            return
        
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
            initialfile=f"gateway_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.writelines(self.log_buffer)
                messagebox.showinfo("Success", f"Log saved to:\n{filename}")
                self.update_status_bar(f"Log saved to {os.path.basename(filename)}")
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save log:\n{str(e)}")
                
    def clear_log(self):
        """Clear log display"""
        if messagebox.askyesno("Clear Log", "Are you sure you want to clear the log?"):
            self.log_buffer.clear()
            self.log_display.config(state='normal')
            self.log_display.delete(1.0, tk.END)
            self.log_display.config(state='disabled')
            self.update_status_bar("Log cleared")
            
    def download_saved_logs(self):
        """Download saved logs from the STM32 device"""
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return
        
        # Clear previous saved logs buffer
        self.saved_logs_buffer.clear()
        self.downloading_saved_logs = True
        self.last_log_time = datetime.now()
        
        # Cancel any existing timeout timer
        if self.download_timeout_timer:
            self.root.after_cancel(self.download_timeout_timer)
        
        # Send the dump command
        self.send_command("DUMP_SAVED_LOGS")
        
        # Show progress message
        self.update_status_bar("Downloading saved logs from device...")
        
    def stop_downloading_logs(self):
        """Stop waiting for downloaded logs"""
        if self.downloading_saved_logs:
            self.downloading_saved_logs = False
            self.last_log_time = None
            
            if self.download_timeout_timer:
                self.root.after_cancel(self.download_timeout_timer)
                self.download_timeout_timer = None
            
            if self.saved_logs_buffer:
                # Save the downloaded logs
                self.save_downloaded_logs()
                self.update_status_bar(f"Downloaded {len(self.saved_logs_buffer)} log lines from device")
            else:
                messagebox.showinfo("No Logs", "No saved logs were received from the device")
                self.update_status_bar("No saved logs received")
    
    def save_downloaded_logs(self):
        """Save the downloaded logs to a file"""
        if not self.saved_logs_buffer:
            return
            
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
            initialfile=f"device_saved_logs_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write("=== STM32 SAVED LOGS ===\n")
                    f.write(f"Downloaded at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                    f.write("=" * 50 + "\n\n")
                    f.writelines(self.saved_logs_buffer)
                    
                messagebox.showinfo("Success", f"Saved {len(self.saved_logs_buffer)} log lines to:\n{filename}")
                self.update_status_bar(f"Device logs saved to {os.path.basename(filename)}")
                
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save logs:\n{str(e)}")
                
    def save_combined_logs(self):
        """Save both GUI logs and downloaded device logs to a single file"""
        if not self.log_buffer and not self.saved_logs_buffer:
            messagebox.showinfo("Empty Logs", "No logs to save")
            return
        
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
            initialfile=f"combined_logs_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write("=== COMBINED LOGS: GUI + STM32 DEVICE ===\n")
                    f.write(f"Generated at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                    f.write("=" * 60 + "\n\n")
                    
                    f.write("=== GUI LOGS (Real-time) ===\n")
                    f.write(f"Total lines: {len(self.log_buffer)}\n\n")
                    f.writelines(self.log_buffer)
                    
                    f.write("\n\n" + "=" * 60 + "\n")
                    f.write("=== STM32 DEVICE LOGS (Saved) ===\n")
                    f.write(f"Total lines: {len(self.saved_logs_buffer)}\n\n")
                    f.writelines(self.saved_logs_buffer)
                    
                messagebox.showinfo("Success", f"Combined logs saved to:\n{filename}")
                self.update_status_bar(f"Combined logs saved to {os.path.basename(filename)}")
                
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save combined logs:\n{str(e)}")
                
    def clear_downloaded_logs(self):
        """Clear the downloaded logs buffer"""
        if self.saved_logs_buffer:
            if messagebox.askyesno("Clear Downloaded Logs", f"Are you sure you want to clear {len(self.saved_logs_buffer)} downloaded log lines?"):
                self.saved_logs_buffer.clear()
                self.update_status_bar("Downloaded logs cleared")
        else:
            messagebox.showinfo("No Logs", "No downloaded logs to clear")
                
    def update_status_bar(self, message):
        """Update status bar message"""
        self.status_bar.config(text=message)
        
    def process_queue(self):
        """Process messages from background threads"""
        try:
            while True:
                msg_type, msg_data = self.message_queue.get_nowait()
                
                if msg_type == 'log':
                    # Determine tag based on content
                    tag = 'INFO'
                    if 'ERROR' in msg_data or 'Error' in msg_data:
                        tag = 'ERROR'
                    elif 'WARN' in msg_data or 'Warning' in msg_data:
                        tag = 'WARN'
                    elif 'DEBUG' in msg_data or 'Debug' in msg_data:
                        tag = 'DEBUG'
                    elif 'SIGNAL DETECTED' in msg_data:
                        tag = 'DETECT'
                    
                    self.append_log(msg_data, tag)
                    
                elif msg_type == 'error':
                    self.append_log(msg_data, 'ERROR')
                    
        except queue.Empty:
            pass
        
        # Schedule next check
        self.root.after(100, self.process_queue)
        
    def on_closing(self):
        """Handle window closing"""
        if self.connected:
            self.disconnect()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = GatewayControlGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()


def test_frame_building():
    """Test frame building functionality"""
    print("Testing Frame Building...")
    
    # Create a GUI instance for testing (without showing the window)
    root = tk.Tk()
    root.withdraw()  # Hide the window
    app = GatewayControlGUI(root)
    
    # Test basic frame building
    frame1 = app.build_binary_command(0x70)  # START_SCAN
    print(f"Basic frame (START_SCAN): {frame1.hex()}")
    assert len(frame1) == 9, f"Expected frame length 9, got {len(frame1)}"
    assert frame1[0] == 0x7E and frame1[-1] == 0x7F, "Frame should start with 0x7E and end with 0x7F"
    
    # Test frame with data
    test_data = struct.pack('<I', 1000)  # 32-bit integer
    frame2 = app.build_binary_command(0x75, test_data)  # SET_SCAN_TIMEOUT
    print(f"Frame with data (SET_SCAN_TIMEOUT): {frame2.hex()}")
    assert len(frame2) == 13, f"Expected frame length 13, got {len(frame2)}"
    
    # Test build method (FrameBuilder pattern)
    frame3 = app.build(0x00, 0x00, 0x70)  # SERVER, ID 0, START_SCAN
    print(f"FrameBuilder pattern (build method): {frame3.hex()}")
    assert frame3 == frame1, "build() and build_binary_command() should produce same result"
    
    # Test CRC calculation
    test_bytes = b'\x00\x00\x70\x00\x00'
    crc = app._calculate_crc(test_bytes)
    print(f"CRC for {test_bytes.hex()}: {crc.hex()}")
    assert len(crc) == 2, "CRC should be 2 bytes"
    
    # Test known CRC values (XMODEM)
    empty_crc = app._calculate_crc(b'')
    print(f"CRC for empty data: {empty_crc.hex()}")
    assert empty_crc == b'\xff\xff', f"Empty CRC should be ffff, got {empty_crc.hex()}"
    
    print("All tests passed!")
    root.destroy()


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        test_frame_building()
    else:
        main()
