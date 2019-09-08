# OpenHantek6022 Translation

## Quick HowTo

1. Go to the translation directory `OpenHantek6022/openhantek/res/translations`.

2. Prepare a translation for a new language or update the translation for an already existing language 
after adding or changing the text (e.g. use `<LANG> = fr` for a french translation):

   `lupdate -recursive ../../src -ts openhantek_<LANG>.ts`

3. Translate the strings using linguist, unprocessed strings remain untranslated:

   `linguist openhantek_<LANG>.ts`

4. For a newly added language update the entries in the project file `openhantek.pro` accordingly.

5. Create the binary translation:

   `lrelease openhantek.pro`

6. For a newly added language update the entries in the resource file `../translations.qrc` accordingly.

7. Go to the `build` directory and issue these commands:

   `cmake ../`
   `make clean`
   `make -j2`


Use the option `--translated` to have the program localized: `OpenHantek --translated`
