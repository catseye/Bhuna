SUBDIR=lib modules driver

strip:
	( cd lib && make strip )
	( cd modules && make strip )
	( cd driver && make strip )

static:
	( cd driver && make static )

.include <bsd.subdir.mk>
