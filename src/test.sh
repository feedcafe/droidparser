#!/bin/bash

./configure &&make&&sudo make install

for bt_config in $(find test -name 'bt_config.xml'); do
	droidparser "$bt_config"
done
