#!/bin/bash

LANGUAGES="openhantek_de.ts openhantek_fr.ts openhantek_pt.ts"

LUPDATE="lupdate -recursive"

# uncomment next line to remove old obsolete strings
# LUPDATE="${LUPDATE} -noobsolete"

SRC=../../src

for LANGUAGE in $LANGUAGES; do
    ${LUPDATE} ${SRC} -ts $LANGUAGE
done

# prepare step 3 (lrelease)
echo "TRANSLATIONS = ${LANGUAGES}" > translations.pro

# collect resources for binary build
QRC=translations.qrc
echo '<RCC>' > $QRC
echo '    <qresource prefix="/translations">' >> $QRC
for LANGUAGE in $LANGUAGES; do
    echo "        <file>${LANGUAGE/ts/qm}</file>" >> $QRC
done
echo '    </qresource>' >> $QRC
echo '</RCC>' >> $QRC

