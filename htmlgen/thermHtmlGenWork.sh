#!/bin/sh
GENDIR="/usr/local/bin/XBeeThermClient"
DFDIR="/etc/XBeeThermClient"
DESTSSH="score@score.webfactional.com:/home/score/webapps/macore"

DGTMPLT="template_dg.html"
DGBASEFN="current_dg"

GCTMPLT="template_gchart.html"
GCBASEFN="current"

GEN="thermHtmlGen"
TMPDIR="/var/tmp"
#TMPDIR="/data/localsite/www"
SCP="/usr/bin/scp"

$GENDIR/$GEN --template=$DFDIR/$GCTMPLT > $TMPDIR/$GCBASEFN.html
$GENDIR/$GEN --template=$DFDIR/$GCTMPLT --days=7 > $TMPDIR/$GCBASEFN\_week.html
$GENDIR/$GEN --template=$DFDIR/$GCTMPLT --days=30 > $TMPDIR/$GCBASEFN\_month.html

$SCP $TMPDIR/$GCBASEFN.html $DESTSSH
$SCP $TMPDIR/$GCBASEFN\_week.html $DESTSSH
$SCP $TMPDIR/$GCBASEFN\_month.html $DESTSSH

$GENDIR/$GEN --template=$DFDIR/$DGTMPLT > $TMPDIR/$DGBASEFN.html
$GENDIR/$GEN --template=$DFDIR/$DGTMPLT --days=7 > $TMPDIR/$DGBASEFN\_week.html
$GENDIR/$GEN --template=$DFDIR/$DGTMPLT --days=30 > $TMPDIR/$DGBASEFN\_month.html

$SCP $TMPDIR/$DGBASEFN.html $DESTSSH
$SCP $TMPDIR/$DGBASEFN\_week.html $DESTSSH
$SCP $TMPDIR/$DGBASEFN\_month.html $DESTSSH

