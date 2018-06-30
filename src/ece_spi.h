// SPI support

// Written by Bernie Roehl, February 2017

void SPI_setup(void);
void SPI_select(void);
void SPI_deselect(void);
uint8_t SPI_xfer(uint8_t byte);
void SPI_writeRegister(uint8_t address, uint8_t data);
uint8_t SPI_readRegister(uint8_t address);
void SPI_readBytes(uint8_t address, uint8_t* buffer, int nbytes);

