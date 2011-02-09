////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dsoextractfw.c
//  Copyright (C) 2008  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010, 2011  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <bfd.h>


int extractFirmware(const char *filenameDriver, const char *filenameFirmware, const char *filenameLoader);
int writeIntelHex(const char *filename, unsigned char *data, unsigned int length);

static const char *filenameEndFirmware = "-firmware.hex";
static const char *filenameEndLoader = "-loader.hex";
static const char *filenameEndDriver = "1.sys";
static const char *nameTarget = "pei-i386";
static const char *nameSection = ".data";
static const char *nameSymbolFirmware = "_firmware";
static const char *nameSymbolLoader = "_loader";


/// \brief Parse commandline arguments.
/// \return 0 on success, negative on error.
int main(int argc, char **argv) {
	char *filenameDriver, *filenameFirmware, *filenameLoader;
	char *charPointer;
	int prefixLength;
	
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <input> [<firmware>] [<loader>]\n", argv[0]);
		return -1;
	}
	filenameDriver = argv[1];
	
	prefixLength = strlen(filenameDriver) - strlen(filenameEndDriver);
	
	if(argc < 3) {
		// Guess correct filename for firmware
		filenameFirmware = malloc(prefixLength + strlen(filenameEndFirmware) + 1);
		memcpy(filenameFirmware, filenameDriver, prefixLength);
		strcpy(filenameFirmware + prefixLength, filenameEndFirmware);
		
		// Convert filename to lowercase
		charPointer = strrchr(filenameFirmware, '/');
		if(charPointer == NULL)
			charPointer = filenameFirmware;
		for(; *charPointer != 0; charPointer++)
			*charPointer = tolower(*charPointer);
	}
	else {
		filenameFirmware = argv[2];
	}
	
	if(argc < 4) {
		// Guess correct filename for loader
		filenameLoader = malloc(prefixLength + strlen(filenameEndLoader) + 1);
		memcpy(filenameLoader, filenameDriver, prefixLength);
		strcpy(filenameLoader + prefixLength, filenameEndLoader);
		
		// Convert filename to lowercase
		charPointer = strrchr(filenameLoader, '/');
		if(charPointer == NULL)
			charPointer = filenameLoader;
		for(; *charPointer != 0; charPointer++)
			*charPointer = tolower(*charPointer);
	}
	else {
		filenameFirmware = argv[3];
	}
	
	return extractFirmware(filenameDriver, filenameFirmware, filenameLoader);
}

/// \brief Extract firmware and loader data from original driver file.
/// \param filenameDriver Name of the original driver file.
/// \param filenameFirmware Name of the file where the firmware should be saved.
/// \param filenameDriver Name of the file where the loader should be saved.
/// \return 0 on success, negative on error.
int extractFirmware(const char *filenameDriver, const char *filenameFirmware, const char *filenameLoader) {
	bfd *bfdDriver;
	asection *sectionData;
	
	asymbol **symbols;
	unsigned int symbolCount;
	unsigned currentSymbol;
	const char *symbolName;
	
	bfd_size_type offsetFirmware = 0, offsetLoader = 0;
	bfd_size_type lengthFirmware = 0, lengthLoader = 0;
	unsigned char *bufferFirmware, *bufferLoader;
	
	// Initialize bfd and open driver file
	bfd_init();
	bfdDriver = bfd_openr(filenameDriver, nameTarget);
	if(!bfdDriver) {
		bfd_perror("Error opening file");
		return -1;
	}
	
	if(!bfd_check_format(bfdDriver, bfd_object)) {
		bfd_perror("bfd_check_format");
		bfd_close(bfdDriver);
		return -2;
	}
	
	// Search for the data section
	for(sectionData = bfdDriver->sections; sectionData != NULL; sectionData = sectionData->next)
		if(strcmp(sectionData->name, nameSection) == 0)
			break;
	if(sectionData == NULL) {
		fprintf(stderr, "Section %s not found\n", nameSection);
		return -3;
	}
	printf("Section %s found (starting at 0x%04lx, %li bytes)\n", nameSection, (unsigned long int) sectionData->filepos, (long int) sectionData->size);
	
	// Search for the symbols we want
	symbols = malloc(bfd_get_symtab_upper_bound(bfdDriver));
	symbolCount = bfd_canonicalize_symtab(bfdDriver, symbols);
	for(currentSymbol = 0; currentSymbol < symbolCount; currentSymbol++) {
		symbolName = bfd_asymbol_name(symbols[currentSymbol]);
		
		if(strcmp(symbolName, nameSymbolFirmware) == 0)
			offsetFirmware = symbols[currentSymbol]->value;
		if(strcmp(symbolName, nameSymbolLoader) == 0)
			offsetLoader = symbols[currentSymbol]->value;
	}
	free(symbols);
	
	// Calculate position in section and length
	offsetFirmware -= sectionData->filepos;
	offsetLoader -= sectionData->filepos;
	lengthFirmware = offsetLoader - offsetFirmware;
	lengthLoader = sectionData->size - lengthFirmware;
	
	printf("Symbol %s found (offset 0x%04lx, %li bytes)\n", nameSymbolFirmware, (unsigned long int) offsetFirmware, (long int) lengthFirmware);
	printf("Symbol %s found (offset 0x%04lx, %li bytes)\n", nameSymbolLoader, (unsigned long int) offsetLoader, (long int) lengthLoader);
	
	// Extract data
	bufferFirmware = malloc(lengthFirmware);
	bufferLoader = malloc(lengthLoader);
	if (bufferFirmware == NULL || bufferLoader == NULL) {
		fprintf(stderr, "Can't allocate memory\n");
		bfd_close(bfdDriver);
		return -4;
	}
	
	if(!bfd_get_section_contents(bfdDriver, sectionData, bufferFirmware, offsetFirmware, lengthFirmware)) {
		bfd_perror("Can't get firmware contents");
		bfd_close(bfdDriver);
		return -5;
	}
	
	if(!bfd_get_section_contents(bfdDriver, sectionData, bufferLoader, offsetLoader, lengthLoader)) {
		bfd_perror("Can't get loader contents");
		bfd_close(bfdDriver);
		return -6;
	}
	
	printf("Saving firmware as %s\n", filenameFirmware);
	writeIntelHex(filenameFirmware, bufferFirmware, lengthFirmware);
	free(bufferFirmware);
	
	printf("Saving loader as %s\n", filenameLoader);
	writeIntelHex(filenameLoader, bufferLoader, lengthLoader);
	free(bufferLoader);
	
	bfd_close(bfdDriver);
	
	return 0;
}

/// \brief Save data to a file in intel hex format.
/// \param filename Name of the output file.
/// \param data Pointer to the binary data that should be stored.
/// \param length Size of the data that should be stored.
/// \return 0 on success, negative on error.
int writeIntelHex(const char *filename, unsigned char *data, unsigned int length)
{
	FILE *file;
	unsigned char crc, eof;
	unsigned int dataIndex, byteIndex, byteCount;
	unsigned char *dataPointer;
	
	file = fopen(filename, "wt");
	if(!file) {
		fprintf(stderr, "Can't open %s for writing\n", filename);
		fclose(file);
		return -1;
	}
	
	for(dataIndex = 0; dataIndex < length; dataIndex += 22) {
		eof = -1; // Always check for End of File Record
		dataPointer = data + dataIndex;
		
		// Start code and byte count
		byteCount = *dataPointer;
		fprintf(file, ":%02X", byteCount);
		if(byteCount != 0)
			eof = 0;
		crc = -*dataPointer++;
		dataPointer++;
		
		// Address
		fprintf(file, "%04X", *(unsigned short *) dataPointer);
		if(*(unsigned short *) dataPointer != 0)
			eof = 0;
		crc -= *dataPointer++;
		crc -= *dataPointer++;
		
		// Record type
		fprintf(file, "%02X", *dataPointer);
		if(*dataPointer != 0x01)
			eof = 0;
		crc -= *dataPointer++;
		
		// Data
		for(byteIndex = 0; byteIndex < byteCount; byteIndex++) {
			fprintf(file, "%02X", *dataPointer);
			crc -= *dataPointer++;
		}
		
		// CRC
		fprintf(file, "%02X\n", crc);
		
		if(eof)
			break;
	}
	
	fclose(file);
	
	return 0;
}
