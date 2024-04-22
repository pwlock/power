AUXFS_PATH=build/auxfs.tar
AUXFS_FILES+=auxfs/em.txt
AUXFS_FILES+=auxfs/driver.autoload

$(AUXFS_PATH): $(AUXFS_FILES)
	tar --transform 's/.*\///g' --format=ustar -cf $@ $^
