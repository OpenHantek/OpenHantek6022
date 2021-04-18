# OpenHantek6022 Translation

The build system simplifies the localization process by automatic preparation of translations. 
All strings in the source code that should be translated must marked with the function `tr()`, instead of 
`QLabel( "Some text" )` use `QLabel( tr( "Some text" ) )`.
All texts marked in this way will be collected automatically during a build into a /translation source/ file.
This file can be translated manually afterwards. The translated texts will be automatically integrated 
into the program during the next build and are available to the user depending on his locale. 
Changed program text will be detected during the next build and the translation source will be updated accordingly.
Text without translation is displayed in the (English) original.

If a translation exists for your language, it will be used automatically.

To use OpenHantek6022 with the original English text, call it with `LANGUAGE= LANG=C OpenHantek`,
friends of Italian ~~Opera~~ language use `LANGUAGE= LANG=it OpenHantek`.
On some systems (e.g. KDE) unsetting `LANGUAGE` with `LANGUAGE=` is necessary because this variable hides the effect of `LANG=xx`.

## Quick HowTo

### Add a new language

Go to the translation directory `openhantek/translations`.

Add your new language to the `set(TS_FILES ...` line near the top of [`Translations.cmake`](https://github.com/OpenHantek/OpenHantek6022/blob/main/openhantek/translations/Translations.cmake), e.g. `openhantek_fr.ts` for French.
```
###################################
#
# Add more languages here
#
set(TS_FILES translations/openhantek_de.ts translations/openhantek_fr.ts)
#
###################################

```
Go to the `build` directory and call `make -j4`
During the build process all translatable text will be added/updated in all TS_FILES from above.

### Translate into your language

Go to the translation directory `openhantek/translations`.
Start translating the original (English) text using linguist, unprocessed strings remain untranslated:

`linguist openhantek_fr.ts`


### Create a binary with new localization

Go to the `build` directory and call `make -j4`

