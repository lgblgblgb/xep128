FILES	= xep128.js xep128.html.mem xep128.data data/xep128-boot.js

all: $(FILES)

xep128.js: ../../xep128.js
	cp $< $@
	git add $@

xep128.html.mem: ../../xep128.html.mem
	cp $< $@
	git add $@

xep128.data: ../../xep128.data
	cp $< $@
	git add $@

data/xep128-boot.js: ../../data/xep128-boot.js
	cp $< $@
	git add $@

commit: $(FILES)
	git commit -m "Update on-line web-demo"
	git push

.PHONY: all commit
