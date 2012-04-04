#!/bin/sh

rsync -Lrtvz --exclude=ko.po  translationproject.org::tp/latest/grub/ po

(
    (
	cd po && ls *.po| cut -d. -f1
	echo "en@quot"
	echo "en@hebrew"
	echo "de@hebrew"
	echo "en@cyrillic"
	echo "en@greek"
	echo "en@arabic"
	echo "en@piglatin"
	echo "de_CH"
    ) | sort | uniq | xargs
) >po/LINGUAS
