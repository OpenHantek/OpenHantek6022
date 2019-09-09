# OpenHantek6022 Translation

## Quick HowTo

Go to the translation directory `openhantek/res/translations`.

### Linux users 

3 simple shell scripts simplify the translation:
```
1.LUPDATE.sh
2.LINGUIST.sh 
3.LRELEASE.sh
```
Add your new language to the `LANGUAGES` line of `1.LUPDATE.sh`
```
LANGUAGES="openhantek_de.ts openhantek_fr.ts openhantek_pt.ts"
```
adapt your language in `2.LINGUIST.sh` 
```
linguist openhantek_de.ts
```
and call the 3 scripts in sequence.

### Other operating systems

Do it step by step:

1. Update the translation for an already existing language after adding or changing the text 
or prepare a translation for a new language (use e.g. `<LANG> = fr` for a french translation):

`lupdate -recursive ../../src -ts openhantek_<LANG>.ts`

2. Translate the strings using linguist, unprocessed strings remain untranslated:

`linguist openhantek_<LANG>.ts`

3. For a newly added language update the entries in the project file `translations.pro` accordingly.
```
TRANSLATIONS = openhantek_de.ts openhantek_fr.ts openhantek_pt.ts
```

4. Create the binary translation:

`lrelease openhantek.pro`

5. For a newly added language update the entries in the resource file `translations.qrc` accordingly.
```
<RCC>
    <qresource prefix="/translations">
        <file>openhantek_de.qm</file>
        <file>openhantek_fr.qm</file>
        <file>openhantek_pt.qm</file>
    </qresource>
</RCC>
```

### Create a new binary

Go to the `build` directory and call `make -j2`

Use the option `--translated` to have the program localized: `OpenHantek --translated`
