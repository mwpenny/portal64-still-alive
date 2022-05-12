#!smake
# --------------------------------------------------------------------
#        Copyright (C) 1998 Nintendo. (Originated by SGI)
#        
#        $RCSfile: Makefile,v $
#        $Revision: 1.1.1.1 $
#        $Date: 2002/05/02 03:27:21 $
# --------------------------------------------------------------------
include /usr/include/n64/make/PRdefs

MIDICVT:=tools/midicvt
SFZ2N64:=tools/sfz2n64
SKELATOOL64:=tools/skeletool64
VTF2PNG:=tools/vtf2png

OPTIMIZER		:= -O0
LCDEFS			:= -DDEBUG -g -Isrc/ -I/usr/include/n64/nustd -Werror -Wall
N64LIB			:= -lultra_rom -lnustd

ifeq ($(WITH_DEBUGGER),1)
LCDEFS += -DWITH_DEBUGGER
endif

BASE_TARGET_NAME = build/portal

LD_SCRIPT	= portal.ld
CP_LD_SCRIPT	= build/portal

ASMFILES    =	$(shell find asm/ -type f -name '*.s')

ASMOBJECTS  =	$(patsubst %.s, build/%.o, $(ASMFILES))

CODEFILES = $(shell find src/ -type f -name '*.c')

CODESEGMENT =	build/codesegment

BOOT		=	/usr/lib/n64/PR/bootcode/boot.6102
BOOT_OBJ	=	build/boot.6102.o

OBJECTS		=	$(ASMOBJECTS) $(BOOT_OBJ)

DEPS = $(patsubst %.c, build/%.d, $(CODEFILES)) $(patsubst %.c, build/%.d, $(DATAFILES))

-include $(DEPS)

LCINCS =	-I/usr/include/n64/PR 
LCDEFS +=	-DF3DEX_GBI_2
#LCDEFS +=	-DF3DEX_GBI_2 -DFOG
#LCDEFS +=	-DF3DEX_GBI_2 -DFOG -DXBUS
#LCDEFS +=	-DF3DEX_GBI_2 -DFOG -DXBUS -DSTOP_AUDIO

LDIRT  =	$(BASE_TARGET_NAME).elf $(CP_LD_SCRIPT) $(BASE_TARGET_NAME).z64 $(BASE_TARGET_NAME)_no_debug.map $(ASMOBJECTS)

LDFLAGS =	-L/usr/lib/n64 $(N64LIB)  -L$(N64_LIBGCCDIR) -lgcc

default:	$(BASE_TARGET_NAME).z64

include $(COMMONRULES)

.s.o:
	$(AS) -Wa,-Iasm -o $@ $<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MM $^ -MF "$(@:.o=.d)" -MT"$@"
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: %.s
	@mkdir -p $(@D)
	$(AS) -Wa,-Iasm -o $@ $<

####################
## Assets
####################

src/models/shadow_caster.h src/models/shadow_caster_geo.inc.h: assets/fbx/ShadowCaster.fbx
	skeletool64 -s 100 -r 0,0,0 -n shadow_caster -o src/models/shadow_caster.h assets/fbx/ShadowCaster.fbx

src/models/ground.h src/models/ground_geo.inc.h: assets/fbx/Ground.fbx
	skeletool64 -s 100 -r 0,0,0 -n ground -o src/models/ground.h assets/fbx/Ground.fbx

src/models/subject.h src/models/subject_geo.inc.h: assets/fbx/Subject.fbx
	skeletool64 -s 100 -r 0,0,0 -n subject -o src/models/subject.h assets/fbx/Subject.fbx

src/models/sphere.h src/models/sphere_geo.inc.h: assets/fbx/Sphere.fbx
	skeletool64 -s 100 -r 0,0,0 -n sphere -o src/models/sphere.h assets/fbx/Sphere.fbx


####################
## vpk extraction
####################

portal_pak_dir: vpk/portal_pak_dir.vpk
	vpk -x portal_pak_dir vpk/portal_pak_dir.vpk

	portal_pak_dir/materials/concrete/concrete_modular_wall001d.png


TEXTURE_SCRIPTS = $(shell find assets/ -type f -name '*.ims')
TEXTURE_IMAGES = $(TEXTURE_SCRIPTS:assets/%.ims=portal_pak_modified/%.png)

%.png: %.vtf
	$(VTF2PNG) $< $@

portal_pak_modified/%.png: portal_pak_dir/%.png assets/%.ims
	@mkdir -p $(@D)
	convert $< $(shell cat $(@:portal_pak_modified/%.png=assets/%.ims)) $@


####################
## Materials
####################

build/assets/materials/static.h build/assets/materials/static_mat.c: assets/materials/static.skm.yaml $(TEXTURE_IMAGES)
	@mkdir -p $(@D)
	$(SKELATOOL64) -n static -m $< -M build/assets/materials/static.h

build/assets/materials/hud.h build/assets/materials/hud_mat.c: assets/materials/hud.skm.yaml $(TEXTURE_IMAGES)
	@mkdir -p $(@D)
	$(SKELATOOL64) -n hud -m $< -M build/assets/materials/hud.h

src/levels/level_def_gen.h: build/assets/materials/static.h

build/src/scene/hud.o: build/assets/materials/hud.h

####################
## Models
####################
#
# Source engine scale is 64x
#

MODEL_LIST = assets/models/cube/cube.blend \
	assets/models/portal_gun/v_portalgun.blend

MODEL_HEADERS = $(MODEL_LIST:%.blend=build/%.h)
MODEL_OBJECTS = $(MODEL_LIST:%.blend=build/%_geo.o)

build/assets/models/%.h build/assets/models/%_geo.c: build/assets/models/%.fbx assets/materials/objects.skm.yaml
	$(SKELATOOL64) -s 2.56 -n $(<:build/assets/models/%.fbx=%) -m assets/materials/objects.skm.yaml -o $(<:%.fbx=%.h) $<

build/src/models/models.o: $(MODEL_HEADERS)

####################
## Test Chambers
####################

TEST_CHAMBERS = $(shell find assets/test_chambers -type f -name '*.blend')

TEST_CHAMBER_HEADERS = $(TEST_CHAMBERS:%.blend=build/%.h)
TEST_CHAMBER_OBJECTS = $(TEST_CHAMBERS:%.blend=build/%_geo.o)

build/%.fbx: %.blend
	@mkdir -p $(@D)
	$(BLENDER_2_9) $< --background --python tools/export_fbx.py -- $@

build/assets/test_chambers/%.h build/assets/test_chambers/%_geo.c: build/assets/test_chambers/%.fbx $(SKELATOOL64) build/assets/materials/static.h
	$(SKELATOOL64) -l -s 2.56 -c 0.01 -n $(<:build/assets/test_chambers/%.fbx=%) -m assets/materials/static.skm.yaml -o $(<:%.fbx=%.h) $<

build/assets/test_chambers/%.o: build/assets/test_chambers/%.c build/assets/materials/static.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MM $^ -MF "$(@:.o=.d)" -MT"$@"
	$(CC) $(CFLAGS) -c -o $@ $<
	
build/assets/materials/%_mat.o: build/assets/materials/%_mat.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MM $^ -MF "$(@:.o=.d)" -MT"$@"
	$(CC) $(CFLAGS) -c -o $@ $<

levels: $(TEST_CHAMBER_HEADERS)
	echo $(TEST_CHAMBER_HEADERS)

build/assets/test_chambers/level_list.h: $(TEST_CHAMBER_HEADERS) tools/generate_level_list.js
	@mkdir -p $(@D)
	node tools/generate_level_list.js $@ $(TEST_CHAMBER_HEADERS)

build/src/levels/levels.o: build/assets/test_chambers/level_list.h build/assets/materials/static.h

.PHONY: levels

####################
## Linking
####################

$(BOOT_OBJ): $(BOOT)
	$(OBJCOPY) -I binary -B mips -O elf32-bigmips $< $@

# without debugger

CODEOBJECTS = $(patsubst %.c, build/%.o, $(CODEFILES)) $(TEST_CHAMBER_OBJECTS) $(MODEL_OBJECTS) build/assets/materials/static_mat.o build/assets/materials/hud_mat.o

CODEOBJECTS_NO_DEBUG = $(CODEOBJECTS)

ifeq ($(WITH_DEBUGGER),1)
CODEOBJECTS_NO_DEBUG += build/debugger/debugger_stub.o build/debugger/serial.o 
endif

$(CODESEGMENT)_no_debug.o:	$(CODEOBJECTS_NO_DEBUG)
	$(LD) -o $(CODESEGMENT)_no_debug.o -r $(CODEOBJECTS_NO_DEBUG) $(LDFLAGS)


$(CP_LD_SCRIPT)_no_debug.ld: $(LD_SCRIPT)
	cpp -P -Wno-trigraphs $(LCDEFS) -DCODE_SEGMENT=$(CODESEGMENT)_no_debug.o -o $@ $<

$(BASE_TARGET_NAME).z64: $(CODESEGMENT)_no_debug.o $(OBJECTS) $(CP_LD_SCRIPT)_no_debug.ld
	$(LD) -L. -T $(CP_LD_SCRIPT)_no_debug.ld -Map $(BASE_TARGET_NAME)_no_debug.map -o $(BASE_TARGET_NAME).elf
	$(OBJCOPY) --pad-to=0x100000 --gap-fill=0xFF $(BASE_TARGET_NAME).elf $(BASE_TARGET_NAME).z64 -O binary
	makemask $(BASE_TARGET_NAME).z64

# with debugger
CODEOBJECTS_DEBUG = $(CODEOBJECTS) 

ifeq ($(WITH_DEBUGGER),1)
CODEOBJECTS_DEBUG += build/debugger/debugger.o build/debugger/serial.o 
endif

$(CODESEGMENT)_debug.o:	$(CODEOBJECTS_DEBUG)
	$(LD) -o $(CODESEGMENT)_debug.o -r $(CODEOBJECTS_DEBUG) $(LDFLAGS)

$(CP_LD_SCRIPT)_debug.ld: $(LD_SCRIPT)
	cpp -P -Wno-trigraphs $(LCDEFS) -DCODE_SEGMENT=$(CODESEGMENT)_debug.o -o $@ $<

$(BASE_TARGET_NAME)_debug.z64: $(CODESEGMENT)_debug.o $(OBJECTS) $(CP_LD_SCRIPT)_debug.ld
	$(LD) -L. -T $(CP_LD_SCRIPT)_debug.ld -Map $(BASE_TARGET_NAME)_debug.map -o $(BASE_TARGET_NAME)_debug.elf
	$(OBJCOPY) --pad-to=0x100000 --gap-fill=0xFF $(BASE_TARGET_NAME)_debug.elf $(BASE_TARGET_NAME)_debug.z64 -O binary
	makemask $(BASE_TARGET_NAME)_debug.z64

clean:
	rm -rf build

.SECONDARY: