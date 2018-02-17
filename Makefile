CFLAGS = -Wall -g -O2

#all: freesub.dll freeoverlay.dll
all: freesub.dll toyv24.dll jmdsource.dll gbrptorgb.dll dolphinmagic.dll timecodefps.dll framercache.dll

SDL_bdf.o: SDL_bdf.c SDL_bdf.h
	gcc -c SDL_bdf.c $(CFLAGS) -o SDL_bdf.o

libavisynth.a: avisynth.def
	dlltool -d avisynth.def -A -k -l libavisynth.a
	
freesub.o: freesub.c SDL_bdf.h
	gcc -c freesub.c $(CFLAGS) -o freesub.o

freesub.dll: freesub.o SDL_bdf.o libavisynth.a
#gcc freesub.o libavisynth.a -Wall -o freesub.dll -shared -Xlinker --enable-stdcall-fixup
	gcc freesub.o SDL_bdf.o libavisynth.a $(CFLAGS) -o freesub.dll -shared

#freeoverlay.o: freeoverlay.c
#	gcc -c freeoverlay.c $(CFLAGS) -o freeoverlay.o
	
#freeoverlay.dll: freeoverlay.o libavisynth.a
#	gcc freeoverlay.o libavisynth.a $(CFLAGS) -o freeoverlay.dll -shared

toyv24.o: toyv24.c
	gcc -c toyv24.c $(CFLAGS) -o toyv24.o

toyv24.dll: toyv24.o libavisynth.a
	gcc toyv24.o libavisynth.a $(CFLAGS) -o toyv24.dll -shared

gbrptorgb.o: gbrptorgb.c
	gcc -c gbrptorgb.c $(CFLAGS) -o gbrptorgb.o

gbrptorgb.dll: gbrptorgb.o libavisynth.a
	gcc gbrptorgb.o libavisynth.a $(CFLAGS) -o gbrptorgb.dll -shared

dolphinmagic.o: dolphinmagic.c
	gcc -c dolphinmagic.c $(CFLAGS) -o dolphinmagic.o

dolphinmagic.dll: dolphinmagic.o libavisynth.a
	gcc dolphinmagic.o libavisynth.a $(CFLAGS) -o dolphinmagic.dll -shared

timecodefps.o: timecodefps.c
	gcc -c timecodefps.c $(CFLAGS) -o timecodefps.o

timecodefps.dll: timecodefps.o libavisynth.a
	gcc timecodefps.o libavisynth.a $(CFLAGS) -o timecodefps.dll -shared

	
jmdsource.o: jmdsource.c
	gcc -c jmdsource.c $(CFLAGS) -o jmdsource.o

jmdsource.dll: jmdsource.o libavisynth.a
	gcc jmdsource.o libavisynth.a $(CFLAGS) -o jmdsource.dll -shared -lz

framercache.o: framercache.c
	gcc -c framercache.c $(CFLAGS) -o framercache.o

framercache.dll: framercache.o libavisynth.a
	gcc framercache.o libavisynth.a $(CFLAGS) -o framercache.dll -shared
