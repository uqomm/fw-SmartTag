// Memory.tpp


template<typename T>
T Memory::getValue(uint8_t key) {
	MemoryLocation location;
	location = value_addr[key];
	return (this->EEPROM_byte_Read<T>(location.address));
}


template<typename T>
void Memory::setValue(uint8_t key, T value) {
    MemoryLocation location;
    if (value_addr.size() > key) {
        location = value_addr[key];
        if (sizeof(T) == location.size)
            this->EEPROM_byte_Write(location.address, value);
    }
}

template<typename T>
T Memory::EEPROM_byte_Read(uint8_t address) {
    T data = 0;
    int size = sizeof(T);
    int i;

    for (i = size - 1; i >= 0; i--) {
        if (i < size - 1) {
            HAL_Delay(5);
            data = data << 8;
        }
        data |= EEPROM_Read(address + i);
    }

    return (data);
}

template<typename T>
void Memory::EEPROM_byte_Write(uint8_t addr, T data) {
    int i;
    int size = sizeof(T);

    for (i = 0; i < size; i++, data = data >> 8) {
        EEPROM_Write(addr + i, static_cast<uint8_t>(data & 0xff));
        HAL_Delay(5);
    }
}