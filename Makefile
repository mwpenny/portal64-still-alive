#!smake
# --------------------------------------------------------------------
#        Copyright (C) 1998 Nintendo. (Originated by SGI)
#        
#        $RCSfile: Makefile,v $
#        $Revision: 1.1.1.1 $
#        $Date: 2002/05/02 03:27:21 $
# --------------------------------------------------------------------
include /usr/include/n64/make/PRdefs


SKELATOOL64:=skelatool64/skeletool64
VTF2PNG:=vtf2png
SFZ2N64:=sfz2n64

$(SKELATOOL64):
	skelatool64/setup_dependencies.sh
	make -C skelatool64

OPTIMIZER		:= -O0
LCDEFS			:= -DDEBUG -g -Isrc/ -I/usr/include/n64/nustd -Werror -Wall
N64LIB			:= -lultra_rom -lnustd

ifeq ($(PORTAL64_WITH_DEBUGGER),1)
LCDEFS += -DPORTAL64_WITH_DEBUGGER
endif

BASE_TARGET_NAME = build/portal

LD_SCRIPT	= portal.ld
CP_LD_SCRIPT	= build/portal

ASMFILES    =	$(shell find asm/ -type f -name '*.s')

ASMOBJECTS  =	$(patsubst %.s, build/%.o, $(ASMFILES))

CODEFILES = $(shell find src/ -type f -name '*.c')

ifeq ($(PORTAL64_WITH_GFX_VALIDATOR),1)
LCDEFS += -DPORTAL64_WITH_GFX_VALIDATOR
CODEFILES += gfxvalidator/validator.c gfxvalidator/error_printer.c gfxvalidator/command_printer.c
endif

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
	vpk -x portal_pak_dir vpk/hl2_sound_misc_dir.vpk


TEXTURE_SCRIPTS = $(shell find assets/ -type f -name '*.ims')
TEXTURE_IMAGES = $(TEXTURE_SCRIPTS:assets/%.ims=portal_pak_modified/%.png)
TEXTURE_VTF_SOURCES = $(TEXTURE_SCRIPTS:assets/%.ims=portal_pak_dir/%.vtf)

ALL_VTF_IMAGES = $(shell find portal_pak_dir/ -type f ! -wholename '* *' -name '*.vtf')
ALL_PNG_IMAGES = $(ALL_VTF_IMAGES:%.vtf=%.png)

$(TEXTURE_VTF_SOURCES): portal_pak_dir

%.png: %.vtf
	-$(VTF2PNG) $< $@

convert_all_png: $(ALL_PNG_IMAGES)

portal_pak_dir/%_copy_0.png: portal_pak_dir/%.png
	cp $< $@

portal_pak_dir/%_copy_1.png: portal_pak_dir/%.png
	cp $< $@

portal_pak_dir/%_copy_2.png: portal_pak_dir/%.png
	cp $< $@

portal_pak_modified/%.png: portal_pak_dir/%.png assets/%.ims
	@mkdir -p $(@D)
	convert $< $(shell cat $(@:portal_pak_modified/%.png=assets/%.ims)) $@


####################
## Materials
####################

build/assets/materials/static.h build/assets/materials/static_mat.c: assets/materials/static.skm.yaml $(TEXTURE_IMAGES) $(SKELATOOL64)
	@mkdir -p $(@D)
	$(SKELATOOL64) --name static -m $< --material-output -o build/assets/materials/static.h

build/assets/materials/ui.h build/assets/materials/ui_mat.c: assets/materials/ui.skm.yaml $(TEXTURE_IMAGES) $(SKELATOOL64)
	@mkdir -p $(@D)
	$(SKELATOOL64) --name ui --default-material default_ui -m $< --material-output -o build/assets/materials/ui.h

build/assets/materials/images.h build/assets/materials/images_mat.c: assets/materials/images.skm.yaml $(TEXTURE_IMAGES) $(SKELATOOL64)
	@mkdir -p $(@D)
	$(SKELATOOL64) --name images --default-material default_ui -m $< --material-output -o build/assets/materials/images.h

build/assets/materials/hud.h build/assets/materials/hud_mat.c: assets/materials/hud.skm.yaml $(TEXTURE_IMAGES) $(SKELATOOL64)
	@mkdir -p $(@D)
	$(SKELATOOL64) --name hud -m $< --material-output -o build/assets/materials/hud.h

src/levels/level_def_gen.h: build/assets/materials/static.h

build/src/scene/hud.o: build/assets/materials/hud.h

build/src/scene/elevator.o: build/assets/models/props/round_elevator_collision.h \
	build/assets/models/props/round_elevator.h \
	build/assets/models/props/round_elevator_interior.h \
	build/assets/materials/static.h

####################
## Models
####################
#
# Source engine scale is 64x
#

MODEL_LIST = assets/models/cube/cube.blend \
	assets/models/player/chell.blend \
	assets/models/portal_gun/v_portalgun.blend \
	assets/models/portal_gun/w_portalgun.blend \
	assets/models/props/autoportal_frame/autoportal_frame.blend \
	assets/models/props/button.blend \
	assets/models/props/door_01.blend \
	assets/models/props/door_02.blend \
	assets/models/props/combine_ball_catcher.blend \
	assets/models/props/combine_ball_launcher.blend \
	assets/models/props/cylinder_test.blend \
	assets/models/props/radio.blend \
	assets/models/props/round_elevator.blend \
	assets/models/props/round_elevator_interior.blend \
	assets/models/props/round_elevator_collision.blend \
	assets/models/props/signage.blend \
	assets/models/props/signage_off.blend \
	assets/models/props/switch001.blend \
	assets/models/props/box_dropper.blend \
	assets/models/props/box_dropper_glass.blend \
	assets/models/portal/portal_blue.blend \
	assets/models/portal/portal_blue_filled.blend \
	assets/models/portal/portal_blue_face.blend \
	assets/models/portal/portal_orange.blend \
	assets/models/portal/portal_orange_filled.blend \
	assets/models/portal/portal_orange_face.blend \
	assets/models/pedestal.blend \
	assets/models/grav_flare.blend \
	assets/models/fleck_ash2.blend

ANIM_LIST = build/assets/models/pedestal_anim.o \
	build/assets/models/props/box_dropper_anim.o \
	build/assets/models/props/combine_ball_catcher_anim.o \
	build/assets/models/props/combine_ball_launcher_anim.o \
	build/assets/models/props/door_01_anim.o \
	build/assets/models/props/door_02_anim.o \
	build/assets/models/player/chell_anim.o \
	build/assets/models/props/switch001_anim.o

MODEL_HEADERS = $(MODEL_LIST:%.blend=build/%.h)
MODEL_OBJECTS = $(MODEL_LIST:%.blend=build/%_geo.o)

build/assets/models/%.h build/assets/models/%_geo.c build/assets/models/%_anim.c: build/assets/models/%.fbx assets/models/%.flags assets/materials/elevator.skm.yaml assets/materials/objects.skm.yaml assets/materials/static.skm.yaml $(TEXTURE_IMAGES) $(SKELATOOL64)
	$(SKELATOOL64) --fixed-point-scale 256 --model-scale 0.01 --name $(<:build/assets/models/%.fbx=%) $(shell cat $(<:build/assets/models/%.fbx=assets/models/%.flags)) -o $(<:%.fbx=%.h) $<

build/src/models/models.o: $(MODEL_HEADERS)

build/src/decor/decor_object_list.o: $(MODEL_HEADERS)

build/src/scene/portal.o: $(MODEL_HEADERS)

build/src/scene/signage.o: $(MODEL_HEADERS)

build/src/scene/box_dropper.o: $(MODEL_HEADERS)

build/src/scene/pedestal.o: $(MODEL_HEADERS)

build/src/scene/render_plan.o: $(MODEL_HEADERS)

build/src/scene/portal_render.o: $(MODEL_HEADERS)

build/src/scene/switch.o: build/assets/models/props/switch001.h build/assets/materials/static.h

build/src/player/player.o: build/assets/models/player/chell.h build/assets/materials/static.h

build/src/scene/ball.o: build/assets/models/grav_flare.h build/assets/models/fleck_ash2.h build/assets/materials/static.h

build/src/scene/ball_launcher.o: build/assets/models/props/combine_ball_launcher.h build/assets/materials/static.h

build/src/scene/ball_catcher.o: build/assets/models/props/combine_ball_catcher.h build/assets/materials/static.h

build/src/scene/door.o: build/assets/models/props/door_01.h build/assets/models/props/door_02.h

build/src/menu/game_menu.o: build/src/audio/clips.h build/assets/materials/ui.h build/assets/materials/images.h build/assets/test_chambers/test_chamber_00/test_chamber_00.h

build/src/menu/main_menu.o: build/src/audio/clips.h build/assets/materials/ui.h build/assets/materials/images.h build/assets/test_chambers/test_chamber_00/test_chamber_00.h

build/src/menu/new_game_menu.o: build/src/audio/clips.h build/assets/materials/ui.h build/assets/materials/images.h build/assets/test_chambers/test_chamber_00/test_chamber_00.h

build/src/menu/load_game.o: build/assets/materials/ui.h build/src/audio/clips.h

build/src/menu/save_game_menu.o: build/src/audio/clips.h

build/src/menu/savefile_list.o: build/assets/materials/ui.h build/src/audio/clips.h

build/src/menu/landing_menu.o: build/assets/materials/ui.h build/src/audio/clips.h

build/src/menu/options_menu.o: build/assets/materials/ui.h

build/src/menu/joystick_options.o: build/assets/materials/ui.h build/src/audio/clips.h

build/src/menu/controls.o: build/assets/materials/ui.h build/src/audio/clips.h

build/assets/models/player/chell.h: assets/materials/chell.skm.yaml

build/assets/models/props/combine_ball_catcher.h: assets/materials/ball_catcher.skm.yaml

build/assets/models/props/combine_ball_launcher.h: assets/materials/ball_catcher.skm.yaml

ANIM_TEST_CHAMBERS = build/assets/test_chambers/test_chamber_00/test_chamber_00_anim.o \
    build/assets/test_chambers/test_chamber_03/test_chamber_03_anim.o \
	build/assets/test_chambers/test_chamber_04/test_chamber_04_anim.o \
	build/assets/test_chambers/test_chamber_06/test_chamber_06_anim.o \
	build/assets/test_chambers/test_chamber_07/test_chamber_07_anim.o

build/anims.ld: $(ANIM_LIST) $(ANIM_TEST_CHAMBERS) tools/generate_animation_ld.js
	@mkdir -p $(@D)
	node tools/generate_animation_ld.js $@ $(ANIM_LIST) $(ANIM_TEST_CHAMBERS)

####################
## Test Chambers
####################

TEST_CHAMBERS = assets/test_chambers/test_chamber_00/test_chamber_00.blend \
	assets/test_chambers/test_chamber_01/test_chamber_01.blend \
	assets/test_chambers/test_chamber_02/test_chamber_02.blend \
	assets/test_chambers/test_chamber_03/test_chamber_03.blend \
	assets/test_chambers/test_chamber_04/test_chamber_04.blend \
	assets/test_chambers/test_chamber_05/test_chamber_05.blend \
	assets/test_chambers/test_chamber_06/test_chamber_06.blend \
	assets/test_chambers/test_chamber_07/test_chamber_07.blend

TEST_CHAMBER_HEADERS = $(TEST_CHAMBERS:%.blend=build/%.h)
TEST_CHAMBER_OBJECTS = $(TEST_CHAMBERS:%.blend=build/%_geo.o)

LUA_FILES = $(shell find tools/ -type f -name '*.lua')

build/%.fbx: %.blend
	@mkdir -p $(@D)
	$(BLENDER_3_0) $< --background --python tools/export_fbx.py -- $@

build/assets/test_chambers/%.h build/assets/test_chambers/%_geo.c build/assets/test_chambers/%_anim.c: build/assets/test_chambers/%.fbx build/assets/materials/static.h $(SKELATOOL64) $(TEXTURE_IMAGES) $(LUA_FILES)
	$(SKELATOOL64) --script tools/export_level.lua --fixed-point-scale 256 --model-scale 0.01 --name $(<:build/assets/test_chambers/%.fbx=%) -m assets/materials/static.skm.yaml -o $(<:%.fbx=%.h) $<

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

build/levels.ld: $(TEST_CHAMBER_OBJECTS) tools/generate_level_ld.js
	@mkdir -p $(@D)
	node tools/generate_level_ld.js $@ $(TEST_CHAMBER_OBJECTS)

build/src/levels/levels.o: build/assets/test_chambers/level_list.h build/assets/materials/static.h

.PHONY: levels

####################
## Sounds
####################

SOUND_ATTRIBUTES = $(shell find assets/ -type f -name '*.sox')
SOUND_JATTRIBUTES = $(shell find assets/ -type f -name '*.jsox')

MUSIC_ATTRIBUTES = $(shell find assets/sound/music/ -type f -name '*.msox')

INS_SOUNDS = $(shell find assets/ -type f -name '*.ins')

SOUND_CLIPS = $(SOUND_ATTRIBUTES:%.sox=build/%.aifc) $(SOUND_JATTRIBUTES:%.jsox=build/%.aifc) $(INS_SOUNDS) $(MUSIC_ATTRIBUTES:%.msox=build/%.aifc)

$(INS_SOUNDS): portal_pak_dir

portal_pak_dir/sound/music/%.wav: portal_pak_dir/sound/music/%.mp3

build/%.aifc: %.sox portal_pak_dir
	@mkdir -p $(@D)
	sox $(<:assets/%.sox=portal_pak_dir/%.wav) $(shell cat $<) $(@:%.aifc=%.wav)
	$(SFZ2N64) -o $@ $(@:%.aifc=%.wav)

build/%.aifc: %.jsox tools/jsox.js portal_pak_dir
	@mkdir -p $(@D)
	node tools/jsox.js $< $(<:assets/%.jsox=portal_pak_dir/%.wav) $(@:%.aifc=%.wav)
	$(SFZ2N64) -o $@ $(@:%.aifc=%.wav)

build/%.aifc: %.msox portal_pak_dir
	@mkdir -p $(@D)
	mpg123 -w $(<:assets/%.msox=portal_pak_dir/%.wav) $(<:assets/%.msox=portal_pak_dir/%.mp3)
	sox $(<:assets/%.msox=portal_pak_dir/%.wav) $(shell cat $<) $(@:%.aifc=%.wav)
	$(SFZ2N64) -o $@ $(@:%.aifc=%.wav)

build/assets/sound/sounds.sounds build/assets/sound/sounds.sounds.tbl: $(SOUND_CLIPS)
	@mkdir -p $(@D)
	$(SFZ2N64) -o $@ $^


build/asm/sound_data.o: build/assets/sound/sounds.sounds build/assets/sound/sounds.sounds.tbl

build/src/audio/clips.h: tools/generate_sound_ids.js $(SOUND_CLIPS)
	@mkdir -p $(@D)
	node tools/generate_sound_ids.js -o $@ -p SOUNDS_ $(SOUND_CLIPS)

build/src/audio/clips.o: build/src/audio/clips.h
build/src/decor/decor_object_list.o: build/src/audio/clips.h

####################
## Linking
####################

$(BOOT_OBJ): $(BOOT)
	$(OBJCOPY) -I binary -B mips -O elf32-bigmips $< $@

# without debugger

CODEOBJECTS = $(patsubst %.c, build/%.o, $(CODEFILES)) $(MODEL_OBJECTS) build/assets/materials/static_mat.o build/assets/materials/ui_mat.o build/assets/materials/hud_mat.o

CODEOBJECTS_NO_DEBUG = $(CODEOBJECTS)

DATA_OBJECTS = build/assets/materials/images_mat.o

ifeq ($(PORTAL64_WITH_DEBUGGER),1)
CODEOBJECTS_NO_DEBUG += build/debugger/debugger_stub.o build/debugger/serial.o 
endif

$(CODESEGMENT)_no_debug.o:	$(CODEOBJECTS_NO_DEBUG)
	$(LD) -o $(CODESEGMENT)_no_debug.o -r $(CODEOBJECTS_NO_DEBUG) $(LDFLAGS)


$(CP_LD_SCRIPT)_no_debug.ld: $(LD_SCRIPT) build/levels.ld build/anims.ld
	cpp -P -Wno-trigraphs $(LCDEFS) -DCODE_SEGMENT=$(CODESEGMENT)_no_debug.o -o $@ $<

$(BASE_TARGET_NAME).z64: $(CODESEGMENT)_no_debug.o $(OBJECTS) $(DATA_OBJECTS) $(CP_LD_SCRIPT)_no_debug.ld
	$(LD) -L. -T $(CP_LD_SCRIPT)_no_debug.ld -Map $(BASE_TARGET_NAME)_no_debug.map -o $(BASE_TARGET_NAME).elf
	$(OBJCOPY) --pad-to=0x100000 --gap-fill=0xFF $(BASE_TARGET_NAME).elf $(BASE_TARGET_NAME).z64 -O binary
	makemask $(BASE_TARGET_NAME).z64

# with debugger
CODEOBJECTS_DEBUG = $(CODEOBJECTS) 

ifeq ($(PORTAL64_WITH_DEBUGGER),1)
CODEOBJECTS_DEBUG += build/debugger/debugger.o build/debugger/serial.o 
endif

$(CODESEGMENT)_debug.o:	$(CODEOBJECTS_DEBUG)
	$(LD) -o $(CODESEGMENT)_debug.o -r $(CODEOBJECTS_DEBUG) $(LDFLAGS)

$(CP_LD_SCRIPT)_debug.ld: $(LD_SCRIPT) build/levels.ld build/anims.ld
	cpp -P -Wno-trigraphs $(LCDEFS) -DCODE_SEGMENT=$(CODESEGMENT)_debug.o -o $@ $<

$(BASE_TARGET_NAME)_debug.z64: $(CODESEGMENT)_debug.o $(OBJECTS) $(DATA_OBJECTS) $(CP_LD_SCRIPT)_debug.ld
	$(LD) -L. -T $(CP_LD_SCRIPT)_debug.ld -Map $(BASE_TARGET_NAME)_debug.map -o $(BASE_TARGET_NAME)_debug.elf
	$(OBJCOPY) --pad-to=0x100000 --gap-fill=0xFF $(BASE_TARGET_NAME)_debug.elf $(BASE_TARGET_NAME)_debug.z64 -O binary
	makemask $(BASE_TARGET_NAME)_debug.z64

clean:
	rm -rf build

fix:
	wine tools/romfix64.exe build/portal.z64 
.SECONDARY:
