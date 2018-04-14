#!/bin/bash
if [ ! -d "${HOME}/.miniComputer" ];then
	mkdir "${HOME}/.miniComputer"
fi
if [ -e "${HOME}/.miniComputer/minicomputerMemory.txt" ];then
	mv "${HOME}/.miniComputer/minicomputerMemory.txt" "${HOME}/.miniComputer/minicomputerMemory.txt.beforeInstall"
	echo "backed up ${HOME}/.miniComputer/minicomputerMemory.txt"
fi 
if [ -e "${HOME}/.miniComputer/minicomputerMulti.txt" ];then
	mv "${HOME}/.miniComputer/minicomputerMulti.txt" "${HOME}/.miniComputer/minicomputerMulti.txt.beforeInstall"
	echo "backed up ${HOME}/.miniComputer/minicomputerMulti.txt"
fi 
if [ -e "${HOME}/.miniComputer/initsinglesound.txt" ];then
	mv "${HOME}/.miniComputer/initsinglesound.txt" "${HOME}/.miniComputer/initsinglesound.txt.beforeInstall"
	echo "backed up ${HOME}/.miniComputer/initsinglesound.txt"
fi 
	cp "factoryPresets/minicomputerMemory.txt" "${HOME}/.miniComputer/minicomputerMemory.txt"
	cp "factoryPresets/minicomputerMulti.txt" "${HOME}/.miniComputer/minicomputerMulti.txt"
	cp "factoryPresets/initsinglesound.txt" "${HOME}/.miniComputer/initsinglesound.txt"
