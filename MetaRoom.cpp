/*
 *  MetaRoom.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Tue May 25 2004.
 *  Copyright (c) 2004 Alyssa Milburn. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "MetaRoom.h"
#include "World.h"
#include "creaturesImage.h"
#include <assert.h>
#include "SDLBackend.h"

MetaRoom::MetaRoom(int _x, int _y, int _width, int _height, const std::string &back) {
	xloc = _x; yloc = _y; wid = _width; hei = _height; firstback = 0;
	if (!back.empty())
		addBackground(back);
}

void MetaRoom::addBackground(std::string back) {
	caos_assert(!back.empty());
	// TODO: cadv adds backgrounds which have already been added as the default, look into this,
	// should we preserve the default once extra backgrounds have been added and change this to
	// a caos_assert?
	if (backgrounds.find(back) != backgrounds.end()) return;
	
	blkImage *background = dynamic_cast<blkImage *>(world.gallery.getImage(back + ".blk"));
	caos_assert(background);

	backgrounds[back] = background;
	if (!firstback) firstback = background;
}

std::vector<std::string> MetaRoom::backgroundList() {
	std::vector<std::string> b;
	for (std::map<std::string, creaturesImage *>::iterator i = backgrounds.begin(); i != backgrounds.end(); i++)
		b.push_back(i->first);
	return b;
}

blkImage *MetaRoom::getBackground(std::string back) {
	if (back.empty()) {
		blkImage *blk = dynamic_cast<blkImage *>(firstback);
		assert(blk || !firstback);
		return blk;
	}
	if (backgrounds.find(back) != backgrounds.end()) return 0;
	blkImage *blk = dynamic_cast<blkImage *>(backgrounds[back]);
	assert(blk || !backgrounds[back]);
	return blk;
}
	
MetaRoom::~MetaRoom() {
	for (std::vector<Room *>::iterator i = rooms.begin(); i != rooms.end(); i++) {
		delete *i;
	}

	for (std::map<std::string, creaturesImage *>::iterator i = backgrounds.begin(); i != backgrounds.end(); i++) {
		world.gallery.delImage(i->second);
	}
}

unsigned int MetaRoom::addRoom(Room *r) {
	rooms.push_back(r);
	world.map.rooms.push_back(r);
	r->id = world.map.room_base++;
	return r->id;
}

/* vim: set noet: */
