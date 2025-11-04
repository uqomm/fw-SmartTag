#!/usr/bin/env python3
"""
Sniffer-Tag Control GUI - STM32 UWB Tag Scanner Interface
Provides a graphical interface to control the sniffer-tag via UART

Features:
- Send UART commands
- Configure detection modes (ONE_DETECTION / MULTIPLE_DETECTION)
- Configure sleep times and tag readings
- Real-time log viewing with distance and battery monitoring
- Save logs to files
- View statistics (tags detected, readings, distances)
- Control UWB parameters

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
import struct
import os
import re


class SnifferTagGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 Sniffer-Tag Control - UWB Tag Scanner Interface")
        self.root.geometry("1400x900")
        
        # Serial connections
        self.serial_port = None
        self.connected = False
        self.read_thread = None
        self.stop_thread = False
        
        # Message queue for thread-safe GUI updates
        self.message_queue = queue.Queue()
        
        # Log storage
        self.log_buffer = []
        self.max_log_lines = 10000
        
        # Statistics
        self.stats = {
            'tags_detected': 0,
            'total_readings': 0,
            'last_detection_time': None,
            'unique_tags': set(),
            'avg_distance_a': [],
            'avg_distance_b': [],
            'avg_battery': []
        }
        
        # Tag tracking
        self.active_tags = {}  # tag_id -> {last_seen, readings, distance_a, distance_b, battery, detecciones_efectivas, detecciones_erradas}
        
        # Detection history - stores all detections with timestamp
        self.detection_history = []  # list of {timestamp, tag_id, readings, distance_a, distance_b, battery, detecciones_efectivas, detecciones_erradas, detection_type}
        
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
        top_frame.grid(row=0, column=0, columnspan=3, sticky=(tk.W, tk.E))
        
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
        refresh_btn = ttk.Button(top_frame, text="Refresh", command=self.refresh_ports)
        refresh_btn.grid(row=0, column=5, padx=5)
        
        # Connection status
        self.status_label = ttk.Label(top_frame, text="● Disconnected", foreground="red")
        self.status_label.grid(row=0, column=6, padx=20)
        
        # ============================================================
        # MIDDLE FRAME - Main Content (3 columns)
        # ============================================================
        
        # LEFT COLUMN - Controls
        left_frame = ttk.Frame(self.root, padding="10")
        left_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Detection Mode Control
        mode_group = ttk.LabelFrame(left_frame, text="Detection Mode", padding="10")
        mode_group.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Button(mode_group, text="🎯 One Detection", 
                   command=lambda: self.send_command("ONE_DETECTION")).grid(row=0, column=0, padx=5, pady=5, sticky=(tk.W, tk.E))
        ttk.Button(mode_group, text="🔍 Multiple Detection", 
                   command=lambda: self.send_command("MULTIPLE_DETECTION")).grid(row=0, column=1, padx=5, pady=5, sticky=(tk.W, tk.E))
        
        # Configuration Commands
        config_group = ttk.LabelFrame(left_frame, text="Configuration", padding="10")
        config_group.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)
        
        # Sleep time received
        ttk.Label(config_group, text="Sleep Time (Received):").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.sleep_received_entry = ttk.Entry(config_group, width=10)
        self.sleep_received_entry.insert(0, "5")
        self.sleep_received_entry.grid(row=0, column=1, padx=5, pady=2)
        ttk.Button(config_group, text="Set", 
                   command=self.set_sleep_received).grid(row=0, column=2, padx=5)
        
        # Sleep time not received
        ttk.Label(config_group, text="Sleep Time (Not Rcv):").grid(row=1, column=0, sticky=tk.W, padx=5)
        self.sleep_not_received_entry = ttk.Entry(config_group, width=10)
        self.sleep_not_received_entry.insert(0, "10")
        self.sleep_not_received_entry.grid(row=1, column=1, padx=5, pady=2)
        ttk.Button(config_group, text="Set", 
                   command=self.set_sleep_not_received).grid(row=1, column=2, padx=5)
        
        # Tag readings
        ttk.Label(config_group, text="Tag Readings:").grid(row=2, column=0, sticky=tk.W, padx=5)
        self.tag_readings_entry = ttk.Entry(config_group, width=10)
        self.tag_readings_entry.insert(0, "5")
        self.tag_readings_entry.grid(row=2, column=1, padx=5, pady=2)
        ttk.Button(config_group, text="Set", 
                   command=self.set_tag_readings).grid(row=2, column=2, padx=5)
        
        # UWB Timeouts Configuration
        timeout_group = ttk.LabelFrame(left_frame, text="⏱ UWB Timeouts & Delays", padding="10")
        timeout_group.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
        
        # Helper class for tooltips
        class ToolTip:
            def __init__(self, widget, text):
                self.widget = widget
                self.text = text
                self.tooltip = None
                self.widget.bind("<Enter>", self.show_tooltip)
                self.widget.bind("<Leave>", self.hide_tooltip)
            
            def show_tooltip(self, event=None):
                x, y, _, _ = self.widget.bbox("insert")
                x += self.widget.winfo_rootx() + 25
                y += self.widget.winfo_rooty() + 25
                
                self.tooltip = tk.Toplevel(self.widget)
                self.tooltip.wm_overrideredirect(True)
                self.tooltip.wm_geometry(f"+{x}+{y}")
                
                label = tk.Label(self.tooltip, text=self.text, 
                               background="#ffffe0", relief=tk.SOLID, 
                               borderwidth=1, font=('Arial', 9),
                               justify=tk.LEFT, padx=5, pady=3)
                label.pack()
            
            def hide_tooltip(self, event=None):
                if self.tooltip:
                    self.tooltip.destroy()
                    self.tooltip = None
        
        # RX Timeouts Section
        ttk.Label(timeout_group, text="── RX Timeouts (μs) ──", font=('Arial', 9, 'bold')).grid(row=0, column=0, columnspan=3, pady=(0,5))
        
        # PRE_TIMEOUT (Preamble detection)
        pre_label = ttk.Label(timeout_group, text="Preamble Timeout:")
        pre_label.grid(row=1, column=0, sticky=tk.W, padx=5)
        ToolTip(pre_label, "Preamble detection timeout.\nTime to detect signal start.\nHigher values improve detection\nat long range (>20m) but reduce\nefficiency. Critical for weak signals.")
        
        self.pre_timeout_entry = ttk.Entry(timeout_group, width=10)
        self.pre_timeout_entry.insert(0, "12")
        self.pre_timeout_entry.grid(row=1, column=1, padx=5, pady=2)
        
        pre_unit_label = ttk.Label(timeout_group, text="PAC (def: 12)")
        pre_unit_label.grid(row=1, column=2, sticky=tk.W, padx=2)
        ToolTip(pre_unit_label, "PAC = Preamble Acquisition Chunk\nRange: 1-64\nIncreased from 5→12 for\n>20m detection (0%→50% success)")
        
        # RESP RX Timeout
        resp_label = ttk.Label(timeout_group, text="RESP RX Timeout:")
        resp_label.grid(row=2, column=0, sticky=tk.W, padx=5)
        ToolTip(resp_label, "Response reception timeout.\nMax time to wait for RESPONSE\nafter sending POLL.\nIncrease for long range or\nslow tag response.")
        
        self.resp_rx_timeout_entry = ttk.Entry(timeout_group, width=10)
        self.resp_rx_timeout_entry.insert(0, "600")
        self.resp_rx_timeout_entry.grid(row=2, column=1, padx=5, pady=2)
        
        resp_unit_label = ttk.Label(timeout_group, text="μs (def: 600)")
        resp_unit_label.grid(row=2, column=2, sticky=tk.W, padx=2)
        ToolTip(resp_unit_label, "Microseconds\nIncreased from 300→600\nfor >20m detection")
        
        # FINAL RX Timeout
        final_label = ttk.Label(timeout_group, text="FINAL RX Timeout:")
        final_label.grid(row=3, column=0, sticky=tk.W, padx=5)
        ToolTip(final_label, "Final message reception timeout.\nMax time to wait for FINAL\nafter sending RESPONSE.\nCompletes DS-TWR ranging cycle.")
        
        self.final_rx_timeout_entry = ttk.Entry(timeout_group, width=10)
        self.final_rx_timeout_entry.insert(0, "220")
        self.final_rx_timeout_entry.grid(row=3, column=1, padx=5, pady=2)
        
        final_unit_label = ttk.Label(timeout_group, text="μs (def: 220)")
        final_unit_label.grid(row=3, column=2, sticky=tk.W, padx=2)
        ToolTip(final_unit_label, "Microseconds\nTypical value for 6.8 Mbps")
        
        # TX to RX Delays Section
        ttk.Label(timeout_group, text="── TX→RX Delays (μs) ──", font=('Arial', 9, 'bold')).grid(row=4, column=0, columnspan=3, pady=(10,5))
        
        # POLL TX to RESP RX Delay
        poll_tx_label = ttk.Label(timeout_group, text="POLL TX→RESP RX:")
        poll_tx_label.grid(row=5, column=0, sticky=tk.W, padx=5)
        ToolTip(poll_tx_label, "Delay from POLL transmission end\nto RESPONSE reception window open.\nAccounts for signal propagation\nand tag processing time.\nAdjust for distance.")
        
        self.poll_tx_resp_rx_entry = ttk.Entry(timeout_group, width=10)
        self.poll_tx_resp_rx_entry.insert(0, "700")
        self.poll_tx_resp_rx_entry.grid(row=5, column=1, padx=5, pady=2)
        
        poll_tx_unit_label = ttk.Label(timeout_group, text="μs (def: 700)")
        poll_tx_unit_label.grid(row=5, column=2, sticky=tk.W, padx=2)
        ToolTip(poll_tx_unit_label, "Microseconds\nIncrease for long distances")
        
        # RESP TX to FINAL RX Delay
        resp_tx_label = ttk.Label(timeout_group, text="RESP TX→FINAL RX:")
        resp_tx_label.grid(row=6, column=0, sticky=tk.W, padx=5)
        ToolTip(resp_tx_label, "Delay from RESPONSE transmission\nto FINAL reception window open.\nSecond half of DS-TWR cycle.\nMust sync with tag timing.")
        
        self.resp_tx_final_rx_entry = ttk.Entry(timeout_group, width=10)
        self.resp_tx_final_rx_entry.insert(0, "500")
        self.resp_tx_final_rx_entry.grid(row=6, column=1, padx=5, pady=2)
        
        resp_tx_unit_label = ttk.Label(timeout_group, text="μs (def: 500)")
        resp_tx_unit_label.grid(row=6, column=2, sticky=tk.W, padx=2)
        ToolTip(resp_tx_unit_label, "Microseconds\nCritical for accurate ToF")
        
        # RX to TX Delays Section
        ttk.Label(timeout_group, text="── RX→TX Delays (μs) ──", font=('Arial', 9, 'bold')).grid(row=7, column=0, columnspan=3, pady=(10,5))
        
        # POLL RX to RESP TX Delay
        poll_rx_label = ttk.Label(timeout_group, text="POLL RX→RESP TX:")
        poll_rx_label.grid(row=8, column=0, sticky=tk.W, padx=5)
        ToolTip(poll_rx_label, "Delay from POLL reception\nto RESPONSE transmission.\nTag-side processing delay.\nDefines when tag responds.")
        
        self.poll_rx_resp_tx_entry = ttk.Entry(timeout_group, width=10)
        self.poll_rx_resp_tx_entry.insert(0, "900")
        self.poll_rx_resp_tx_entry.grid(row=8, column=1, padx=5, pady=2)
        
        poll_rx_unit_label = ttk.Label(timeout_group, text="μs (def: 900)")
        poll_rx_unit_label.grid(row=8, column=2, sticky=tk.W, padx=2)
        ToolTip(poll_rx_unit_label, "Microseconds\nMust match sniffer expectation")
        
        # RESP RX to FINAL TX Delay
        resp_rx_label = ttk.Label(timeout_group, text="RESP RX→FINAL TX:")
        resp_rx_label.grid(row=9, column=0, sticky=tk.W, padx=5)
        ToolTip(resp_rx_label, "Delay from RESPONSE reception\nto FINAL transmission.\nCompletes ranging sequence.\nMust be coordinated with tag.")
        
        self.resp_rx_final_tx_entry = ttk.Entry(timeout_group, width=10)
        self.resp_rx_final_tx_entry.insert(0, "700")
        self.resp_rx_final_tx_entry.grid(row=9, column=1, padx=5, pady=2)
        
        resp_rx_unit_label = ttk.Label(timeout_group, text="μs (def: 700)")
        resp_rx_unit_label.grid(row=9, column=2, sticky=tk.W, padx=2)
        ToolTip(resp_rx_unit_label, "Microseconds\nCritical for DS-TWR accuracy")
        
        # Inter-Ranging Delay Section
        ttk.Label(timeout_group, text="── Ranging Cycle (ms) ──", font=('Arial', 9, 'bold')).grid(row=10, column=0, columnspan=3, pady=(10,5))
        
        # RNG_DELAY (Inter-ranging delay)
        rng_label = ttk.Label(timeout_group, text="Inter-Ranging Delay:")
        rng_label.grid(row=11, column=0, sticky=tk.W, padx=5)
        ToolTip(rng_label, "Delay between ranging cycles.\nControls measurement frequency.\nLower = faster updates, higher power.\nHigher = slower updates, lower power.\nBalance based on application needs.")
        
        self.rng_delay_entry = ttk.Entry(timeout_group, width=10)
        self.rng_delay_entry.insert(0, "1000")
        self.rng_delay_entry.grid(row=11, column=1, padx=5, pady=2)
        
        rng_unit_label = ttk.Label(timeout_group, text="ms (def: 1000)")
        rng_unit_label.grid(row=11, column=2, sticky=tk.W, padx=2)
        ToolTip(rng_unit_label, "Milliseconds\nRange: 100-10000\n1 Hz update rate @ 1000ms")
        
        ttk.Button(timeout_group, text="Apply All UWB Parameters", 
                   command=self.set_uwb_timeouts).grid(row=12, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))
        
        # Info label
        info_label = ttk.Label(timeout_group, text="⚠ Higher values improve long-range detection\nbut increase response time and power consumption", 
                              font=('Arial', 8), foreground='gray')
        info_label.grid(row=13, column=0, columnspan=3, pady=5)
        
        # Quick Commands
        commands_group = ttk.LabelFrame(left_frame, text="Quick Commands", padding="10")
        commands_group.grid(row=3, column=0, sticky=(tk.W, tk.E), pady=5)
        
        quick_commands = [
            ("Query LTEL", "QUERY_LTEL"),
            ("Query Sigma", "QUERY_SIGMA"),
            ("Query STR", "QUERY_STR"),
            ("Query ADC", "QUERY_ADC"),
            ("Set ATT LTEL", "SET_ATT_LTEL"),
            ("Set POUT Max", "SET_POUT_MAX"),
            ("Set POUT Min", "SET_POUT_MIN"),
        ]
        
        for idx, (label, cmd) in enumerate(quick_commands):
            row = idx // 2
            col = idx % 2
            ttk.Button(commands_group, text=label, 
                       command=lambda c=cmd: self.send_command(c)).grid(
                           row=row, column=col, padx=5, pady=3, sticky=(tk.W, tk.E))
        
        # Custom Command
        custom_group = ttk.LabelFrame(left_frame, text="Custom Command", padding="10")
        custom_group.grid(row=4, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(custom_group, text="Command (hex):").grid(row=0, column=0, padx=5)
        self.custom_cmd_entry = ttk.Entry(custom_group, width=15)
        self.custom_cmd_entry.grid(row=0, column=1, padx=5)
        self.custom_cmd_entry.bind('<Return>', lambda e: self.send_custom_command())
        
        ttk.Button(custom_group, text="Send", 
                   command=self.send_custom_command).grid(row=0, column=2, padx=5)
        
        # Statistics
        stats_group = ttk.LabelFrame(left_frame, text="Statistics", padding="10")
        stats_group.grid(row=5, column=0, sticky=(tk.W, tk.E), pady=5)
        
        self.stats_text = tk.Text(stats_group, height=12, width=40, state='disabled', font=('Consolas', 9))
        self.stats_text.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        ttk.Button(stats_group, text="Refresh Stats", 
                   command=self.update_statistics).grid(row=1, column=0, pady=5, sticky=(tk.W, tk.E))
        ttk.Button(stats_group, text="Clear Stats", 
                   command=self.clear_statistics).grid(row=2, column=0, pady=2, sticky=(tk.W, tk.E))
        
        # MIDDLE COLUMN - Active Tags
        middle_frame = ttk.Frame(self.root, padding="10")
        middle_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        tags_label_frame = ttk.Frame(middle_frame)
        tags_label_frame.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        ttk.Label(tags_label_frame, text="Active Tags:", font=('Arial', 10, 'bold')).grid(row=0, column=0, sticky=tk.W)
        ttk.Button(tags_label_frame, text="🗑 Clear Tags", 
                   command=self.clear_active_tags).grid(row=0, column=1, padx=5)
        ttk.Button(tags_label_frame, text="💾 Export History", 
                   command=self.export_detection_history).grid(row=0, column=2, padx=5)
        ttk.Button(tags_label_frame, text="🗑 Clear History", 
                   command=self.clear_detection_history).grid(row=0, column=3, padx=5)
        
        # Active tags display with treeview
        self.tags_tree = ttk.Treeview(middle_frame, columns=('ID', 'Readings', 'Dist A', 'Dist B', 'Battery', 'Efectivas', 'Erradas', 'Last Seen'), 
                                       show='headings', height=25)
        self.tags_tree.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        # Configure columns
        self.tags_tree.heading('ID', text='Tag ID')
        self.tags_tree.heading('Readings', text='Readings')
        self.tags_tree.heading('Dist A', text='Dist A (m)')
        self.tags_tree.heading('Dist B', text='Dist B (m)')
        self.tags_tree.heading('Battery', text='Battery')
        self.tags_tree.heading('Efectivas', text='Efectivas')
        self.tags_tree.heading('Erradas', text='Erradas')
        self.tags_tree.heading('Last Seen', text='Last Seen')
        
        self.tags_tree.column('ID', width=80, anchor='center')
        self.tags_tree.column('Readings', width=70, anchor='center')
        self.tags_tree.column('Dist A', width=80, anchor='center')
        self.tags_tree.column('Dist B', width=80, anchor='center')
        self.tags_tree.column('Battery', width=70, anchor='center')
        self.tags_tree.column('Efectivas', width=70, anchor='center')
        self.tags_tree.column('Erradas', width=70, anchor='center')
        self.tags_tree.column('Last Seen', width=120, anchor='center')
        
        # Scrollbar for tags tree
        tags_scrollbar = ttk.Scrollbar(middle_frame, orient=tk.VERTICAL, command=self.tags_tree.yview)
        tags_scrollbar.grid(row=1, column=1, sticky=(tk.N, tk.S))
        self.tags_tree.configure(yscrollcommand=tags_scrollbar.set)
        
        # RIGHT COLUMN - Log Display
        right_frame = ttk.Frame(self.root, padding="10")
        right_frame.grid(row=1, column=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Log controls
        log_control_frame = ttk.Frame(right_frame)
        log_control_frame.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        log_label = ttk.Label(log_control_frame, text="Serial Log:", font=('Arial', 10, 'bold'))
        log_label.grid(row=0, column=0, sticky=tk.W)
        
        save_btn = ttk.Button(log_control_frame, text="💾 Save Log", command=self.save_log)
        save_btn.grid(row=0, column=1, padx=5)
        
        clear_btn = ttk.Button(log_control_frame, text="🗑 Clear Log", command=self.clear_log)
        clear_btn.grid(row=0, column=2, padx=5)
        
        # Auto-scroll checkbox
        auto_scroll_cb = ttk.Checkbutton(log_control_frame, text="Auto-scroll", 
                                          variable=self.auto_scroll_var)
        auto_scroll_cb.grid(row=0, column=3, padx=5)
        
        # Log display
        self.log_display = scrolledtext.ScrolledText(right_frame, width=70, height=45, 
                                                      font=('Consolas', 9), state='disabled')
        self.log_display.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        # Configure text tags for colored output
        self.log_display.tag_config('INFO', foreground='green')
        self.log_display.tag_config('WARN', foreground='orange')
        self.log_display.tag_config('ERROR', foreground='red')
        self.log_display.tag_config('DEBUG', foreground='gray')
        self.log_display.tag_config('TAG', foreground='blue', font=('Consolas', 9, 'bold'))
        self.log_display.tag_config('DISTANCE', foreground='purple')
        
        # ============================================================
        # BOTTOM FRAME - Status Bar
        # ============================================================
        bottom_frame = ttk.Frame(self.root, padding="5")
        bottom_frame.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E))
        
        self.status_bar = ttk.Label(bottom_frame, text="Ready", relief=tk.SUNKEN)
        self.status_bar.pack(fill=tk.X)
        
        # Refresh ports after GUI is created
        self.refresh_ports()
        
        # Configure grid weights for resizing
        self.root.columnconfigure(2, weight=1)
        self.root.rowconfigure(1, weight=1)
        right_frame.columnconfigure(0, weight=1)
        right_frame.rowconfigure(1, weight=1)
        middle_frame.columnconfigure(0, weight=1)
        middle_frame.rowconfigure(1, weight=1)
        
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
    
    def build_binary_command(self, command_id, data=b'', module_function=0x10, module_id=0x00):
        """Build a binary command frame using the LTEL protocol format"""
        # Protocol constants
        START_MARK = 0x7E
        END_MARK = 0x7F
        
        # Frame structure: START(1) + MODULE_FUNCTION(1) + MODULE_ID(1) + CMD(1) + DATA_LEN_HIGH(1) + DATA_LEN_LOW(1) + DATA + CRC(2) + END(1)
        data_len = len(data)
        frame = bytearray([START_MARK, module_function, module_id, command_id, 
                          (data_len >> 8) & 0xFF, data_len & 0xFF])
        frame.extend(data)
        frame += self._calculate_crc(frame[1:])
        frame.append(END_MARK)
        
        return bytes(frame)
    
    def _calculate_crc(self, data: bytes) -> bytes:
        """Calculate CRC16 XMODEM checksum"""
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
                    baudrate = int(self.baudrate_combo.get())
                    self.serial_port = serial.Serial(
                        port=port,
                        baudrate=baudrate,
                        bytesize=serial.EIGHTBITS,
                        parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE,
                        timeout=1
                    )
                    self.update_status_bar(f"Connected to {port} @ {baudrate} baud")
                    
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
        """Send a command to the sniffer-tag using binary protocol"""
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return
        
        try:
            # Map command names to binary command IDs (from CommandMessage.hpp)
            command_map = {
                'QUERY_LTEL': 0x11,
                'QUERY_SIGMA': 0x12,
                'QUERY_STR': 0x15,
                'QUERY_ADC': 0x16,
                'ONE_DETECTION': 0x17,
                'MULTIPLE_DETECTION': 0x18,
                'CHANGE_SLEEP_TIME_RECIVED': 0x19,
                'CHANGE_SLEEP_NOT_RECIVED': 0x20,
                'CHANGE_TAG_READINGS': 0x21,
                'SET_ATT_LTEL': 0x20,
                'SET_POUT_MAX': 0x24,
                'SET_POUT_MIN': 0x23
            }
            
            cmd_id = command_map.get(command)
            if cmd_id is None:
                messagebox.showerror("Unknown Command", f"Unknown command: {command}")
                return
            
            # Build binary frame (MODULE_FUNCTION = 0x10 for SNIFFER)
            frame = self.build_binary_command(cmd_id, module_function=0x10)
            
            # Send the frame
            self.serial_port.write(frame)
            self.append_log(f">>> SENT: CMD=0x{cmd_id:02X} ({command}), Frame={frame.hex()}\n", 'DEBUG')
            
            # Status feedback
            command_names = {
                'QUERY_LTEL': 'Querying LTEL Parameters',
                'QUERY_SIGMA': 'Querying Sigma Parameters',
                'QUERY_STR': 'Querying STR Parameters',
                'QUERY_ADC': 'Querying ADC Values',
                'ONE_DETECTION': 'Setting Mode: ONE Detection',
                'MULTIPLE_DETECTION': 'Setting Mode: MULTIPLE Detection',
                'SET_ATT_LTEL': 'Setting LTEL Attenuation',
                'SET_POUT_MAX': 'Setting Max Power Output',
                'SET_POUT_MIN': 'Setting Min Power Output'
            }
            status_msg = command_names.get(command, f"Sent {command}")
            self.update_status_bar(status_msg)
            
        except Exception as e:
            messagebox.showerror("Send Error", f"Failed to send command:\n{str(e)}")
    
    def set_sleep_received(self):
        """Set sleep time when tag received"""
        try:
            value = int(self.sleep_received_entry.get())
            if value < 0 or value > 255:
                raise ValueError("Value must be 0-255")
            
            data = struct.pack('B', value)
            frame = self.build_binary_command(0x19, data, module_function=0x10)
            self.serial_port.write(frame)
            self.append_log(f">>> SET SLEEP TIME (Received): {value} seconds\n", 'INFO')
            self.update_status_bar(f"Sleep time (received) set to {value}s")
            
        except ValueError as e:
            messagebox.showerror("Invalid Value", str(e))
        except Exception as e:
            messagebox.showerror("Error", f"Failed to set sleep time:\n{str(e)}")
    
    def set_sleep_not_received(self):
        """Set sleep time when tag not received"""
        try:
            value = int(self.sleep_not_received_entry.get())
            if value < 0 or value > 255:
                raise ValueError("Value must be 0-255")
            
            data = struct.pack('B', value)
            frame = self.build_binary_command(0x20, data, module_function=0x10)
            self.serial_port.write(frame)
            self.append_log(f">>> SET SLEEP TIME (Not Received): {value} seconds\n", 'INFO')
            self.update_status_bar(f"Sleep time (not received) set to {value}s")
            
        except ValueError as e:
            messagebox.showerror("Invalid Value", str(e))
        except Exception as e:
            messagebox.showerror("Error", f"Failed to set sleep time:\n{str(e)}")
    
    def set_tag_readings(self):
        """Set number of tag readings"""
        try:
            value = int(self.tag_readings_entry.get())
            if value < 1 or value > 255:
                raise ValueError("Value must be 1-255")
            
            data = struct.pack('B', value)
            frame = self.build_binary_command(0x21, data, module_function=0x10)
            self.serial_port.write(frame)
            self.append_log(f">>> SET TAG READINGS: {value}\n", 'INFO')
            self.update_status_bar(f"Tag readings set to {value}")
            
        except ValueError as e:
            messagebox.showerror("Invalid Value", str(e))
        except Exception as e:
            messagebox.showerror("Error", f"Failed to set tag readings:\n{str(e)}")
    
    def set_uwb_timeouts(self):
        """Set all UWB RX timeouts, TX/RX delays, and ranging parameters"""
        try:
            # Get all values
            pre_timeout = int(self.pre_timeout_entry.get())
            resp_rx_timeout = int(self.resp_rx_timeout_entry.get())
            final_rx_timeout = int(self.final_rx_timeout_entry.get())
            poll_tx_resp_rx = int(self.poll_tx_resp_rx_entry.get())
            resp_tx_final_rx = int(self.resp_tx_final_rx_entry.get())
            poll_rx_resp_tx = int(self.poll_rx_resp_tx_entry.get())
            resp_rx_final_tx = int(self.resp_rx_final_tx_entry.get())
            rng_delay = int(self.rng_delay_entry.get())
            
            # Validate ranges
            if pre_timeout < 1 or pre_timeout > 64:
                raise ValueError("Preamble timeout must be 1-64 PAC units")
            if resp_rx_timeout < 100 or resp_rx_timeout > 10000:
                raise ValueError("RESP RX timeout must be 100-10000 μs")
            if final_rx_timeout < 100 or final_rx_timeout > 10000:
                raise ValueError("FINAL RX timeout must be 100-10000 μs")
            if poll_tx_resp_rx < 100 or poll_tx_resp_rx > 10000:
                raise ValueError("POLL TX→RESP RX delay must be 100-10000 μs")
            if resp_tx_final_rx < 100 or resp_tx_final_rx > 10000:
                raise ValueError("RESP TX→FINAL RX delay must be 100-10000 μs")
            if poll_rx_resp_tx < 100 or poll_rx_resp_tx > 10000:
                raise ValueError("POLL RX→RESP TX delay must be 100-10000 μs")
            if resp_rx_final_tx < 100 or resp_rx_final_tx > 10000:
                raise ValueError("RESP RX→FINAL TX delay must be 100-10000 μs")
            if rng_delay < 100 or rng_delay > 10000:
                raise ValueError("Inter-ranging delay must be 100-10000 ms")
            
            # Note: These commands need to be implemented in firmware
            # Command IDs mapping (to be added in CommandMessage.hpp):
            # 0x30: SET_PRE_TIMEOUT
            # 0x31: SET_RESP_RX_TIMEOUT
            # 0x32: SET_FINAL_RX_TIMEOUT
            # 0x33: SET_POLL_TX_RESP_RX_DELAY
            # 0x34: SET_RESP_TX_FINAL_RX_DELAY
            # 0x35: SET_POLL_RX_RESP_TX_DELAY
            # 0x36: SET_RESP_RX_FINAL_TX_DELAY
            # 0x37: SET_RNG_DELAY
            
            commands = [
                (0x30, pre_timeout, "Preamble Timeout (PAC units)"),
                (0x31, resp_rx_timeout, "RESP RX Timeout"),
                (0x32, final_rx_timeout, "FINAL RX Timeout"),
                (0x33, poll_tx_resp_rx, "POLL TX→RESP RX Delay"),
                (0x34, resp_tx_final_rx, "RESP TX→FINAL RX Delay"),
                (0x35, poll_rx_resp_tx, "POLL RX→RESP TX Delay"),
                (0x36, resp_rx_final_tx, "RESP RX→FINAL TX Delay"),
                (0x37, rng_delay, "Inter-Ranging Delay (ms)"),
            ]
            
            self.append_log(f">>> SET UWB PARAMETERS:\n", 'INFO')
            
            # Send all commands
            for cmd_id, value, description in commands:
                data = struct.pack('<I', value)  # uint32_t little-endian
                frame = self.build_binary_command(cmd_id, data, module_function=0x10)
                self.serial_port.write(frame)
                unit = "PAC" if cmd_id == 0x30 else ("ms" if cmd_id == 0x37 else "μs")
                self.append_log(f"    {description}: {value} {unit}\n", 'INFO')
                time.sleep(0.05)  # Small delay between commands
            
            self.append_log(f"⚠ Note: Firmware must support commands 0x30-0x37\n", 'WARN')
            
            self.update_status_bar(f"UWB parameters configured (8 parameters sent)")
            
            messagebox.showinfo("UWB Parameters", 
                              f"All UWB parameters sent:\n\n"
                              f"RX Timeouts:\n"
                              f"  • Preamble: {pre_timeout} PAC\n"
                              f"  • RESP RX: {resp_rx_timeout} μs\n"
                              f"  • FINAL RX: {final_rx_timeout} μs\n\n"
                              f"TX→RX Delays:\n"
                              f"  • POLL TX→RESP RX: {poll_tx_resp_rx} μs\n"
                              f"  • RESP TX→FINAL RX: {resp_tx_final_rx} μs\n\n"
                              f"RX→TX Delays:\n"
                              f"  • POLL RX→RESP TX: {poll_rx_resp_tx} μs\n"
                              f"  • RESP RX→FINAL TX: {resp_rx_final_tx} μs\n\n"
                              f"Ranging Cycle:\n"
                              f"  • Inter-Ranging Delay: {rng_delay} ms\n\n"
                              f"⚠ Verify firmware supports commands 0x30-0x37")
            
        except ValueError as e:
            messagebox.showerror("Invalid Value", str(e))
        except Exception as e:
            messagebox.showerror("Error", f"Failed to set UWB parameters:\n{str(e)}")
            
    def send_custom_command(self):
        """Send custom command - accepts hex command ID optionally followed by hex data"""
        cmd = self.custom_cmd_entry.get().strip()
        if not cmd:
            return
        
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return
        
        try:
            # Parse command: "0x17" or "17 05" (command ID + optional hex data)
            parts = cmd.split()
            if len(parts) >= 1:
                cmd_id_str = parts[0]
                data_hex = parts[1] if len(parts) > 1 else ""
                
                # Parse command ID
                if cmd_id_str.startswith('0x') or cmd_id_str.startswith('0X'):
                    cmd_id = int(cmd_id_str, 16)
                else:
                    cmd_id = int(cmd_id_str, 16)
                
                # Parse optional data
                data = b''
                if data_hex:
                    data = bytes.fromhex(data_hex)
                
                # Build and send binary frame
                frame = self.build_binary_command(cmd_id, data, module_function=0x10)
                self.serial_port.write(frame)
                self.append_log(f">>> SENT CUSTOM: CMD=0x{cmd_id:02X}, Data={data.hex() if data else 'None'}, Frame={frame.hex()}\n", 'DEBUG')
                self.custom_cmd_entry.delete(0, tk.END)
                self.update_status_bar(f"Sent custom command: 0x{cmd_id:02X}")
            else:
                messagebox.showerror("Invalid Format", "Use format: CMD_ID [DATA]\nExample: 17 or 19 05")
                
        except ValueError as e:
            messagebox.showerror("Parse Error", f"Invalid hex format:\n{str(e)}")
        except Exception as e:
            messagebox.showerror("Send Error", f"Failed to send:\n{str(e)}")
            
    def append_log(self, text, tag='INFO'):
        """Append text to log display"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        formatted_text = f"[{timestamp}] {text}\n"
        
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
        
        # Parse for statistics and tag tracking
        self.parse_log_for_stats(text)
        
    def parse_log_for_stats(self, text):
        """Parse log text for statistics and tag tracking"""
        # Parse tag detection: [HH:MM:SS] TAG [0xXXXX] - R:5, DistA:04.94, DistB:04.76, Bat:34
        tag_pattern = r'TAG \[0x([0-9A-Fa-f]+)\] - R:(\d+), DistA:([\d.]+|N/A), DistB:([\d.]+|N/A), Bat:(\d+)'
        match = re.search(tag_pattern, text)
        
        if match:
            tag_id = match.group(1)
            readings = int(match.group(2))
            dist_a = match.group(3)
            dist_b = match.group(4)
            battery = int(match.group(5))
            
            # Update statistics
            self.stats['tags_detected'] += 1
            self.stats['total_readings'] += readings
            self.stats['last_detection_time'] = datetime.now()
            self.stats['unique_tags'].add(tag_id)
            
            # Track distances and battery
            if dist_a != 'N/A':
                try:
                    self.stats['avg_distance_a'].append(float(dist_a))
                except:
                    pass
            if dist_b != 'N/A':
                try:
                    self.stats['avg_distance_b'].append(float(dist_b))
                except:
                    pass
            self.stats['avg_battery'].append(battery)
            
            # Classify detection as efectiva or errada
            try:
                dist_a_val = float(dist_a) if dist_a != 'N/A' else 0
                dist_b_val = float(dist_b) if dist_b != 'N/A' else 0
                
                # Check if tag already exists to preserve counters
                if tag_id in self.active_tags:
                    efectivas = self.active_tags[tag_id].get('detecciones_efectivas', 0)
                    erradas = self.active_tags[tag_id].get('detecciones_erradas', 0)
                else:
                    efectivas = 0
                    erradas = 0
                
                # Increment counters based on detection quality
                if dist_a_val > 0 and dist_b_val > 0:
                    # Detección efectiva: ambas antenas detectaron
                    efectivas += 1
                else:
                    # Detección errada: solo una antena o ninguna
                    erradas += 1
                    
            except ValueError:
                # Si no se puede parsear, contar como errada
                if tag_id in self.active_tags:
                    efectivas = self.active_tags[tag_id].get('detecciones_efectivas', 0)
                    erradas = self.active_tags[tag_id].get('detecciones_erradas', 0) + 1
                else:
                    efectivas = 0
                    erradas = 1
            
            # Update active tags
            self.active_tags[tag_id] = {
                'readings': readings,
                'distance_a': dist_a,
                'distance_b': dist_b,
                'battery': battery,
                'last_seen': datetime.now(),
                'detecciones_efectivas': efectivas,
                'detecciones_erradas': erradas
            }
            
            # Store detection in history
            detection_type = "efectiva" if (dist_a_val > 0 and dist_b_val > 0) else "errada"
            self.detection_history.append({
                'timestamp': datetime.now(),
                'tag_id': tag_id,
                'readings': readings,
                'distance_a': dist_a,
                'distance_b': dist_b,
                'battery': battery,
                'detecciones_efectivas': efectivas,
                'detecciones_erradas': erradas,
                'detection_type': detection_type
            })
            
            # Update tags display
            self.update_tags_display()
            
            # Auto-update statistics
            self.root.after(100, self.update_statistics)
        
        # Parse "Tags detected: X"
        if "Tags detected:" in text:
            try:
                count = int(text.split("Tags detected:")[1].strip())
                # This is a summary, don't increment stats
            except:
                pass
        
        # Parse "No tag detected"
        if "No tag detected" in text or "No tags detected" in text:
            # Clear active tags that haven't been seen in 30 seconds
            current_time = datetime.now()
            to_remove = []
            for tag_id, info in self.active_tags.items():
                if (current_time - info['last_seen']).total_seconds() > 30:
                    to_remove.append(tag_id)
            
            for tag_id in to_remove:
                del self.active_tags[tag_id]
            
            if to_remove:
                self.update_tags_display()
    
    def update_tags_display(self):
        """Update the active tags treeview"""
        # Clear existing items
        for item in self.tags_tree.get_children():
            self.tags_tree.delete(item)
        
        # Add current tags
        for tag_id, info in sorted(self.active_tags.items()):
            last_seen = info['last_seen'].strftime("%H:%M:%S")
            self.tags_tree.insert('', tk.END, values=(
                f"0x{tag_id}",
                info['readings'],
                info['distance_a'],
                info['distance_b'],
                info['battery'],
                info.get('detecciones_efectivas', 0),
                info.get('detecciones_erradas', 0),
                last_seen
            ))
    
    def clear_active_tags(self):
        """Clear the active tags display"""
        if messagebox.askyesno("Clear Tags", "Clear all active tags?"):
            self.active_tags.clear()
            self.update_tags_display()
            self.update_status_bar("Active tags cleared")
    
    def export_detection_history(self):
        """Export detection history to CSV file"""
        if not self.detection_history:
            messagebox.showinfo("No Data", "No detection history to export")
            return
        
        filename = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
            initialfile=f"detection_history_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        )
        
        if filename:
            try:
                import csv
                with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                    fieldnames = ['timestamp', 'tag_id', 'readings', 'distance_a', 'distance_b', 'battery', 'detecciones_efectivas', 'detecciones_erradas', 'detection_type']
                    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                    
                    # Write header
                    writer.writeheader()
                    
                    # Write data
                    for detection in self.detection_history:
                        writer.writerow({
                            'timestamp': detection['timestamp'].strftime('%Y-%m-%d %H:%M:%S.%f')[:-3],
                            'tag_id': f"0x{detection['tag_id']}",
                            'readings': detection['readings'],
                            'distance_a': detection['distance_a'],
                            'distance_b': detection['distance_b'],
                            'battery': detection['battery'],
                            'detecciones_efectivas': detection['detecciones_efectivas'],
                            'detecciones_erradas': detection['detecciones_erradas'],
                            'detection_type': detection['detection_type']
                        })
                
                messagebox.showinfo("Success", f"Detection history exported to:\n{filename}\n\nTotal detections: {len(self.detection_history)}")
                self.update_status_bar(f"History exported to {os.path.basename(filename)}")
                
            except Exception as e:
                messagebox.showerror("Export Error", f"Failed to export history:\n{str(e)}")
    
    def clear_detection_history(self):
        """Clear the detection history"""
        if not self.detection_history:
            messagebox.showinfo("No Data", "Detection history is already empty")
            return
        
        if messagebox.askyesno("Clear History", f"Clear detection history?\n\nCurrent detections: {len(self.detection_history)}"):
            self.detection_history.clear()
            self.update_status_bar("Detection history cleared")
        
    def update_statistics(self):
        """Update statistics display"""
        avg_dist_a = sum(self.stats['avg_distance_a']) / len(self.stats['avg_distance_a']) if self.stats['avg_distance_a'] else 0
        avg_dist_b = sum(self.stats['avg_distance_b']) / len(self.stats['avg_distance_b']) if self.stats['avg_distance_b'] else 0
        avg_battery = sum(self.stats['avg_battery']) / len(self.stats['avg_battery']) if self.stats['avg_battery'] else 0
        
        stats_text = f"""
Total Detections: {self.stats['tags_detected']}
Unique Tags: {len(self.stats['unique_tags'])}
Total Readings: {self.stats['total_readings']}
Active Tags: {len(self.active_tags)}
History Entries: {len(self.detection_history)}

Average Distance A: {avg_dist_a:.2f} m
Average Distance B: {avg_dist_b:.2f} m
Average Battery: {avg_battery:.0f}

Last Detection:
{self.stats['last_detection_time'].strftime('%Y-%m-%d %H:%M:%S') if self.stats['last_detection_time'] else 'None'}

Log Lines: {len(self.log_buffer)}
        """
        
        self.stats_text.config(state='normal')
        self.stats_text.delete(1.0, tk.END)
        self.stats_text.insert(1.0, stats_text.strip())
        self.stats_text.config(state='disabled')
    
    def clear_statistics(self):
        """Clear statistics"""
        if messagebox.askyesno("Clear Statistics", "Clear all statistics?"):
            self.stats = {
                'tags_detected': 0,
                'total_readings': 0,
                'last_detection_time': None,
                'unique_tags': set(),
                'avg_distance_a': [],
                'avg_distance_b': [],
                'avg_battery': []
            }
            self.update_statistics()
            self.update_status_bar("Statistics cleared")
                
    def save_log(self):
        """Save log to file"""
        # Always allow saving, even if empty
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
            initialfile=f"sniffer_tag_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write("=== SNIFFER-TAG LOG ===\n")
                    f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                    f.write("=" * 60 + "\n\n")
                    
                    if self.log_buffer:
                        f.writelines(self.log_buffer)
                    else:
                        f.write("(No log data captured yet)\n")
                    
                    # Add statistics
                    f.write("\n\n" + "=" * 60 + "\n")
                    f.write("=== STATISTICS ===\n\n")
                    f.write(f"Total Detections: {self.stats['tags_detected']}\n")
                    f.write(f"Unique Tags: {len(self.stats['unique_tags'])}\n")
                    f.write(f"Active Tags: {len(self.active_tags)}\n")
                    
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
                    elif 'DEBUG' in msg_data or 'Debug' in msg_data or '>>>' in msg_data:
                        tag = 'DEBUG'
                    elif 'TAG [0x' in msg_data:
                        tag = 'TAG'
                    elif 'Dist' in msg_data or 'distance' in msg_data:
                        tag = 'DISTANCE'
                    
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
    app = SnifferTagGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()


if __name__ == "__main__":
    main()
