nmake -f Makefile.vc clean
CLMonitor.exe monitor --attach
nmake -f Makefile.vc
mkdir tmp
CLMonitor.exe analyze -l "tmp\swirc.plog" --intermodular
PlogConverter.exe -o "tmp" -a "GA:1,2;64:1;OP:1,2,3;CS:1;OWASP:1" -t FullHtml "tmp\swirc.plog"
