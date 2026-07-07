PDFIO_VERSION := 1.2.0
PDFIO_DIR := pdfio-$(PDFIO_VERSION)
PDFIO_URL := https://github.com/michaelrsweet/pdfio/releases/download/v$(PDFIO_VERSION)/$(PDFIO_DIR).tar.gz

PDFIO_CFLAGS := -fPIC -D__ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__

results.mk:
	if ! [ -f $(PDFIO_DIR).tar.gz ]; then wget $(PDFIO_URL); fi
	if [ -d $(PDFIO_DIR) ]; then rm -rf $(PDFIO_DIR); fi
	tar xf $(PDFIO_DIR).tar.gz
	cd $(PDFIO_DIR) && $(CC) -c $(PDFIO_CFLAGS) $(CFLAGS) pdfio-*.c ttf.c
	cd $(PDFIO_DIR) && $(AR) rcs libpdfio.a pdfio-*.o ttf.o
	echo "LIB_CFLAGS += -I$(CURDIR)/$(PDFIO_DIR)" >> results.mk
	echo "LIB_LDFLAGS += $(CURDIR)/$(PDFIO_DIR)/libpdfio.a -lz" >> results.mk
	touch results.mk
