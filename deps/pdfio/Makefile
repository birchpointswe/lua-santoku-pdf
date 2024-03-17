ifndef <% return var("PDFIO_NOBUILD") %>

<% return var("PDFIO_LDFLAGS") %> ?= $(CURDIR)/pdfio-1.2.0/libpdfio.a -lz
<% return var("PDFIO_CFLAGS") %> ?= -I$(CURDIR)/pdfio-1.2.0

results.mk: Makefile
	[ ! -f pdfio-1.2.0.tar.gz ] && wget "https://github.com/michaelrsweet/pdfio/releases/download/v1.2.0/pdfio-1.2.0.tar.gz" || true
	rm -rf pdfio-1.2.0 || true
	tar xf pdfio-1.2.0.tar.gz
	cd pdfio-1.2.0 && ./configure
	cd pdfio-1.2.0 && make all
	echo "LIB_CFLAGS += $(<% return var("PDFIO_CFLAGS") %>)" > "$@"
	echo "LIB_LDFLAGS += $(<% return var("PDFIO_LDFLAGS") %>)" >> $@

else

results.mk: Makefile
	echo "LIB_CFLAGS += $(<% return var("PDFIO_CFLAGS") %>)" > "$@"
	echo "LIB_LDFLAGS += $(<% return var("PDFIO_LDFLAGS") %>)" >> $@

endif
