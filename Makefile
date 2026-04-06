BE := glfw

LDLIBS := -lm -lgeo -lxxhash -lqmap -lqsys -lqgl
LDLIBS += ${LDLIBS-${BE}}

LDLIBS-Linux += -lEGL

obj-y := game time
obj-y += tile char
obj-y += view map
obj-y += dialog
seven-ways-obj-y := ${obj-y:%=src/%.o}

tmxc-obj-y := map img png
tmxc-obj-y := ${tmxc-obj-y:%=src/%.o}
LDLIBS-tmxc := -ltmx -lz -lxml2

pngs :=  press seven tiles lamb rooster ui font
resources := ${pngs:%=resources/%.png} resources/icon.ico
map := col0 map0 map1 map2 map3
map := ${map:%=map/%.png} map/info.txt
share := map.txt ${resources} ${map}
share-dirs := resources map

CFLAGS := -g -I../qmap/include -I../geo/include -I../qgl/include

# Ensure the linker searches the updated local libraries first
LDFLAGS := -L../qgl -L../qmap -L../geo

-include ../mk/include.mk
