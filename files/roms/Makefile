all:
	for a in *.rom ; do stat --printf="%s\t`md5sum $$a`\n" $$a ; done > README
	cat README | awk 'BEGIN { print "<html><head><h1>ROMs</h1></head><body><table>" } NF == 3 { print "<tr><td align=\"right\"><code>" $$1 "</code></td><td><i><code>" $$2 "</code></i></td><td><b><code><a href=\"" $$3 "\">" $$3 "</a></code></b></td></tr>" } END { print "</table></body></html>" }' > index.html
	git add README Makefile
