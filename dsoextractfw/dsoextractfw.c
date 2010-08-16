////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dsoextractfw.c
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010  Oliver Haag
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


#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <bfd.h>

static const char *strFirmware = "_firmware";
static const char *strLoader = "_loader";
static const char *strHex = ".hex";
static const char *strDriver = "1.SYS";
static const char *strModels[] = { "DSO2090", "DSO2100", "DSO2150", "DSO2250", "DSO5200", "DSO520A", NULL };

int writeSRecords(const char *filename, unsigned char *ptr, bfd_size_type len);
int extractFirmware(const char* model);

int writeSRecords(const char *filename, unsigned char *ptr, bfd_size_type len)
{
	unsigned char n, *p, crc=0;
	bfd_size_type  i, t;
	FILE *f;
	
	if ((f=fopen(filename, "wt")) == NULL)
	{
		perror("Cant' open file for writing");
		fclose(f);
		return -1;
	}
	
	for(t=0; t <len; t+=22)
	{
		p = ptr + t;
		n = *p;
		fprintf(f, ":%02X", n);
		crc = *p++;
		p++;
		fprintf(f, "%04X", *(unsigned short *)p);
		crc += *p++;
		crc += *p++;
		fprintf(f, "%02X", *p);
		crc += *p++;
		for(i=0; i<n; i++)
		{
			fprintf(f,"%02X", *p);
			crc += *p++;
		}
		crc = 1 + ~crc;
		fprintf(f, "%02X\n", crc);
	}
	fclose(f);
	
	return 0;
}

int extractFirmware(const char* model)
{
	bfd *file;
	asection *section;
	unsigned storage;
	asymbol **syms = NULL;
	asymbol *asym;
	unsigned nsyms, i;
	const char *sname;
	bfd_size_type offsetData = 0, lenData = 0;
	bfd_size_type offsetFirmware = 0, offsetLoader = 0;
	bfd_size_type lenFirmware = 0, lenLoader = 0;
	unsigned char *ptrFirmware, *ptrLoader;
	char filename[256];
	
	strcpy(filename, model);
	strcat(filename, strDriver);
	printf("Searching %s\n", filename);
	
	bfd_init();
	file = bfd_openr(filename, 0);//"efi-app-ia32");
	if (!file)
	{
		bfd_perror("Error opening file");
		return -1;
	}
	
	if (!bfd_check_format(file, bfd_object))
	{
		bfd_perror("bfd_check_format");
		bfd_close (file);
		return -1;
	}
	
	section = bfd_get_section_by_name(file, ".data");
	if (section != NULL)
	{
		lenData = section->size;
		offsetData = section->filepos;
		printf(".data section found at %p, length %li bytes\n", (void*)offsetData, lenData);
		
		storage = bfd_get_symtab_upper_bound(file);
		syms = malloc(storage);
		nsyms = bfd_canonicalize_symtab(file, syms);
		for(i=0; i<nsyms; i++)
		{
			asym = syms[i];
			sname = bfd_asymbol_name(asym);
			if (strcmp(sname, strFirmware) == 0)
				offsetFirmware = asym->value;
			if (strcmp(sname, strLoader) == 0)
				offsetLoader = asym->value;
		}
		free(syms);
		
		offsetFirmware -= offsetData;
		offsetLoader -= offsetData;
		lenFirmware = offsetLoader - offsetFirmware;
		lenLoader = lenData - lenFirmware;
		printf("Firmware found at offset 0x%lX, length %li bytes\n", offsetFirmware, lenFirmware);
		printf("Loader found at offset 0x%lX, length %li bytes\n", offsetLoader, lenLoader);
		
		ptrFirmware = malloc(lenFirmware);
		ptrLoader = malloc(lenLoader);
		if (ptrFirmware == NULL || ptrLoader == NULL)
		{
			perror("Can't allocate memory");
			bfd_close(file);
			return -1;
		}
		
		if (!bfd_get_section_contents(file, section, ptrFirmware, offsetFirmware, lenFirmware))
		{
			bfd_perror("Can't get firmware contents");
			bfd_close(file);
			return -1;
		}
		
		if (!bfd_get_section_contents(file, section, ptrLoader, offsetLoader, lenLoader))
		{
			bfd_perror("Can't get loader contents");
			bfd_close(file);
			return -1;
		}
		
		strcpy(filename, model);
		strcat(filename, strFirmware);
		strcat(filename, strHex);
		printf("Writing %s\n", filename);
		writeSRecords(filename, ptrFirmware, lenFirmware);
		
		strcpy(filename, model);
		strcat(filename, strLoader);
		strcat(filename, strHex);
		printf("Writing %s\n", filename);
		writeSRecords(filename, ptrLoader, lenLoader);
		
		free(ptrFirmware);
		free(ptrLoader);
	}
	else
	{
		fprintf(stderr, "Section .data not found\n");
	}
	
	bfd_close(file);
	
	return 0;
}

int main()
{
	int i;
	
	for(i=0; strModels[i]!=NULL; i++)
		extractFirmware(strModels[i]);
	
	return 0;
}
