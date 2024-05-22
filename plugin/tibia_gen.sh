#!/bin/sh

TIBIA_DIR=../../tibia
dir=`dirname $0`

$TIBIA_DIR/tibia $dir/tibia/product.json,$dir/tibia/company.json,$dir/tibia/vst3.json $TIBIA_DIR/templates/vst3 vst3
$TIBIA_DIR/tibia $dir/tibia/product.json,$dir/tibia/company.json,$dir/tibia/vst3.json,$dir/tibia/make.json,$dir/tibia/vst3-make.json $TIBIA_DIR/templates/vst3-make vst3

$TIBIA_DIR/tibia $dir/tibia/product.json,$dir/tibia/company.json,$dir/tibia/lv2.json $TIBIA_DIR/templates/lv2 lv2
$TIBIA_DIR/tibia $dir/tibia/product.json,$dir/tibia/company.json,$dir/tibia/lv2.json,$dir/tibia/make.json,$dir/tibia/lv2-make.json $TIBIA_DIR/templates/lv2-make lv2
