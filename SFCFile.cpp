/*
 *  SFCFile.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Sat 21 Oct 2006.
 *  Copyright (c) 2006 Alyssa Milburn. All rights reserved.
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

#include "SFCFile.h"
#include "exceptions.h"

/*
 * sfcdumper.py has better commentary on this format - use it for debugging
 * and make sure to update it if you update this!
 */

#define TYPE_MAPDATA 1
#define TYPE_CGALLERY 2
#define TYPE_CDOOR 3
#define TYPE_CROOM 4
#define TYPE_ENTITY 5
#define TYPE_COMPOUNDOBJECT 6
#define TYPE_BLACKBOARD 7
#define TYPE_VEHICLE 8
#define TYPE_LIFT 9
#define TYPE_SIMPLEOBJECT 10
#define TYPE_POINTERTOOL 11
#define TYPE_CALLBUTTON 12
#define TYPE_SCENERY 13
#define TYPE_OBJECT 100

SFCFile::~SFCFile() {
	// This contains all the objects we've constructed, so we can just zap this and
	// everything neatly disappears.
	for (std::vector<SFCClass *>::iterator i = storage.begin(); i != storage.end(); i++) {
		delete *i;
	}
}

void SFCFile::read(std::istream *i) {
	ourStream = i;

	mapdata = (MapData *)slurpMFC(TYPE_MAPDATA);
	assert(mapdata);

	// TODO: hackery to seek to the next bit
	uint8 x = 0;
	while (x == 0) x = read8();
	ourStream->seekg(-1, std::ios::cur);

	uint32 numobjects = read32();
	for (unsigned int i = 0; i < numobjects; i++) {
		SFCObject *o = (SFCObject *)slurpMFC(TYPE_OBJECT);
		assert(o);
		objects.push_back(o);
	}

	uint32 numscenery = read32();
	for (unsigned int i = 0; i < numscenery; i++) {
		SFCScenery *o = (SFCScenery *)slurpMFC(TYPE_SCENERY);
		assert(o);
		scenery.push_back(o);
	}

	uint32 numscripts = read32();
	for (unsigned int i = 0; i < numscripts; i++) {
		SFCScript x;
		x.read(this);
		scripts.push_back(x);
	}

	scrollx = read32();
	scrolly = read32();
	
	// TODO
}

bool validSFCType(unsigned int type, unsigned int reqtype) {
	if (reqtype == 0) return true;
	if (type == reqtype) return true;
	if ((reqtype == TYPE_OBJECT) && (type >= TYPE_COMPOUNDOBJECT)) return true;
	if ((reqtype == TYPE_COMPOUNDOBJECT) && (type >= TYPE_COMPOUNDOBJECT) && (type <= TYPE_LIFT)) return true;

	return false;
}

SFCClass *SFCFile::slurpMFC(unsigned int reqtype) {
	assert(!ourStream->fail());

	// read the pid (this only works up to 0x7ffe, but we'll cope)
	uint16 pid = read16();

	if (pid == 0) {
		// null object
		return 0;
	} else if (pid == 0xffff) {
		// completely new class, read details
		uint16 schemaid = read16();
		uint16 strlen = read16();
		char *temp = new char[strlen];
		ourStream->read(temp, strlen);
		std::string classname(temp, strlen);
		delete[] temp;
		
		pid = storage.size();
		
		// push a null onto the stack
		storage.push_back(0);

		// set the types array as necessary
		if (classname == "MapData")
			types[pid] = TYPE_MAPDATA;
		else if (classname == "CGallery")
			types[pid] = TYPE_CGALLERY;
		else if (classname == "CDoor")
			types[pid] = TYPE_CDOOR;
		else if (classname == "CRoom")
			types[pid] = TYPE_CROOM;
		else if (classname == "Entity")
			types[pid] = TYPE_ENTITY;
		else if (classname == "CompoundObject")
			types[pid] = TYPE_COMPOUNDOBJECT;
		else if (classname == "Blackboard")
			types[pid] = TYPE_BLACKBOARD;
		else if (classname == "Vehicle")
			types[pid] = TYPE_VEHICLE;
		else if (classname == "Lift")
			types[pid] = TYPE_LIFT;
		else if (classname == "SimpleObject")
			types[pid] = TYPE_SIMPLEOBJECT;
		else if (classname == "PointerTool")
			types[pid] = TYPE_POINTERTOOL;
		else if (classname == "CallButton")
			types[pid] = TYPE_CALLBUTTON;
		else if (classname == "Scenery")
			types[pid] = TYPE_SCENERY;
		else
			throw creaturesException(std::string("SFCFile doesn't understand class name '") + classname + "'!");
	} else if ((pid & 0x8000) != 0x8000) {
		// return an existing object
		pid -= 1;
		assert(pid < storage.size());
		assert(validSFCType(types[pid], reqtype));
		SFCClass *temp = storage[pid];
		assert(temp);
		return temp;
	} else {
		uint16 oldpid = pid;
		// create a new object of an existing class
		pid ^= 0x8000;
		pid -= 1;
		assert(pid < storage.size());
		assert(!(storage[pid]));
	}

	SFCClass *newobj;

	// construct new object of specified type
	assert(validSFCType(types[pid], reqtype));
	switch (types[pid]) {
		case TYPE_MAPDATA: newobj = new MapData(this); break;
		case TYPE_CGALLERY: newobj = new CGallery(this); break;
		case TYPE_CDOOR: newobj = new CDoor(this); break;
		case TYPE_CROOM: newobj = new CRoom(this); break;
		case TYPE_ENTITY: newobj = new SFCEntity(this); break;
		case TYPE_COMPOUNDOBJECT: newobj = new SFCCompoundObject(this); break;
		case TYPE_BLACKBOARD: newobj = new SFCBlackboard(this); break;
		case TYPE_VEHICLE: newobj = new SFCVehicle(this); break;
		case TYPE_LIFT: newobj = new SFCLift(this); break;
		case TYPE_SIMPLEOBJECT: newobj = new SFCSimpleObject(this); break;
		case TYPE_POINTERTOOL: newobj = new SFCPointerTool(this); break;
		case TYPE_CALLBUTTON: newobj = new SFCCallButton(this); break;
		case TYPE_SCENERY: newobj = new SFCScenery(this); break;
		default:
			throw creaturesException("SFCFile didn't find a valid type in internal variable, argh!");
	}

	if (validSFCType(types[pid], TYPE_COMPOUNDOBJECT)) reading_compound = true;
	else if (types[pid] == TYPE_SCENERY) reading_scenery = true;

	// push the object onto storage, and make it deserialize itself
	types[storage.size()] = types[pid];
	storage.push_back(newobj);
	newobj->read();

	if (validSFCType(types[pid], TYPE_COMPOUNDOBJECT)) reading_compound = false;
	else if (types[pid] == TYPE_SCENERY) reading_scenery = false;

	// return this new object
	return newobj;
}

uint8 SFCFile::read8() {
	char temp[1];
	ourStream->read(temp, 1);
	return temp[0];
}

uint16 SFCFile::read16() {
	char temp[2];
	ourStream->read(temp, 2);
	uint16 *i = (uint16 *)&temp;
	return swapEndianShort(*i);
}

uint32 SFCFile::read32() {
	char temp[4];
	ourStream->read(temp, 4);
	uint32 *i = (uint32 *)&temp;
	return swapEndianLong(*i);
}

std::string SFCFile::readstring() {
	uint32 strlen = read8();
	if (strlen == 0xff) {
		strlen = read16();
		if (strlen == 0xffff)
			strlen = read32();
	}

	return readBytes(strlen);
}

std::string SFCFile::readBytes(unsigned int n) {
	char *temp = new char[n];
	ourStream->read(temp, n);
	std::string t = std::string(temp, n);
	delete[] temp;
	return t;
}

// ------------------------------------------------------------------

void MapData::read() {
	// discard unknown bytes
	assert(read16() == 1);
	assert(read16() == 0);
	read32(); read32(); read32();

	background = (CGallery *)slurpMFC(TYPE_CGALLERY);
	assert(background);
	uint32 norooms = read32();
	for (unsigned int i = 0; i < norooms; i++) {
		CRoom *temp = (CRoom*)slurpMFC(TYPE_CROOM);
		assert(temp);
		rooms.push_back(temp);
	}
}

void CGallery::read() {
	noframes = read32();
	filename = parent->readBytes(4);
	firstimg = read32();
	
	// discard unknown bytes
	read32();

	for (unsigned int i = 0; i < noframes; i++) {
		// discard unknown bytes
		read8(); read8(); read8();
		// discard width, height and offset
		read32(); read32(); read32();
	}
}

void CDoor::read() {
	openness = read8();
	otherroom = read16();
	
	// discard unknown bytes
	assert(read16() == 0);
}

void CRoom::read() {
	id = read32();

	// magic constant?
	assert(read16() == 2);

	left = read32();
	top = read32();
	right = read32();
	bottom = read32();

	for (unsigned int i = 0; i < 4; i++) {
		uint16 nodoors = read16();
		for (unsigned int j = 0; j < nodoors; j++) {
			CDoor *temp = (CDoor*)slurpMFC(TYPE_CDOOR);
			assert(temp);
			doors[i].push_back(temp);
		}
	}

	roomtype = read32(); assert(roomtype < 4);

	floorvalue = read8();
	inorganicnutrients = read8();
	organicnutrients = read8();
	temperature = read8();
	heatsource = reads32();

	pressure = read8();
	pressuresource = reads32();

	windx = reads32();
	windy = reads32();

	lightlevel = read8();
	lightsource = reads32();

	radiation = read8();
	radiationsource = reads32();

	// discard unknown bytes
	readBytes(800);

	uint16 nopoints = read16();
	for (unsigned int i = 0; i < nopoints; i++) {
		floorpoints.push_back(std::pair<uint32, uint32>(read32(), read32()));
	}

	// discard unknown bytes
	assert(read32() == 0);

	music = readstring();
	dropstatus = read32(); assert(dropstatus < 3);
}

void SFCEntity::read() {
	// read sprite
	sprite = (CGallery *)slurpMFC(TYPE_CGALLERY);
	assert(sprite);

	// read current frame and offset from base
	currframe = read8();
	imgoffset = read8();

	// read zorder, x, y
	zorder = reads32();
	x = read32();
	y = read32();

	// check if this agent is animated at present
	uint8 animbyte = read8();
	if (animbyte) {
		assert(animbyte == 1);
		haveanim = true;

		// read the animation frame
		animframe = read8();

		// read the animation string
		std::string tempstring = readBytes(99);
		// chop off non-null-terminated bits
		animstring = std::string(tempstring.c_str());
	} else haveanim = false;

	if (parent->readingScenery()) return;

	// read part zorder
	partzorder = read32();
	//if (zorder != partzorder) assert(parent->readingCompound());

	// TODO: read over unknown click bhvr bytes
	readBytes(3);

	// read BHVR touch
	bhvrtouch = read8();

	if (parent->readingCompound()) return;

	// read pickup handles/points
	uint16 num_pickup_handles = read16();
	for (unsigned int i = 0; i < num_pickup_handles; i++) {
		pickup_handles.push_back(std::pair<uint32, uint32>(read32(), read32()));
	}
	uint16 num_pickup_points = read16();
	for (unsigned int i = 0; i < num_pickup_points; i++) {
		pickup_points.push_back(std::pair<uint32, uint32>(read32(), read32()));
	}
}

void SFCObject::read() {
	// read genus, family and species
	genus = read8();
	family = read8();
	assert(read16() == 0);
	species = read16();

	// read UNID
	unid = read32();

	// discard unknown bytes
	read8();

	// read ATTR
	attr = read16();

	// discard unknown bytes
	assert(read16() == 0);

	// read unknown coords
	left = read32();
	top = read32();
	right = read32();
	bottom = read32();

	// discard unknown bytes
	read16();

	// read BHVR click state
	bhvrclickstate = read8();

	// read sprite
	sprite = (CGallery *)slurpMFC(TYPE_CGALLERY);
	assert(sprite);

	// TODO: read over unknown tick bytes
	tick1 = read32();
	tick2 = read32();

	// discard unknown bytes
	assert(read16() == 0);
	read32();

	// read object variables
	for (unsigned int i = 0; i < 100; i++)
		variables[i] = read32();

	// read physics values
	size = read8();
	range = read32();
	
	// discard unknown bytes
	read32();

	// read physics values
	accg = read32();
	velx = reads32();
	vely = reads32();
	rest = read32();
	aero = read32();

	// discard unknown bytes
	readBytes(6);

	// read threats
	threat = read8();

	// read flags
	uint8 flags = read8();
	frozen = (flags & 0x02);

	// read scripts
	uint32 numscripts = read32();
	for (unsigned int i = 0; i < numscripts; i++) {
		SFCScript x;
		x.read(parent);
		scripts.push_back(x);
	}
}

void SFCCompoundObject::read() {
	SFCObject::read();

	uint32 numparts = read32();

	for (unsigned int i = 0; i < numparts; i++) {
		SFCEntity *e = (SFCEntity *)slurpMFC(TYPE_ENTITY);
		if (!e) {
			// if entity is null, discard unknown bytes
			readBytes(8);
		}

		// push the entity, even if it is null..
		parts.push_back(e);
	}

	// discard hotspot data for now
	readBytes(150);
}

void SFCBlackboard::read() {
	SFCCompoundObject::read();

	// read text x/y position and colours
	textx = read32();
	texty = read32();
	backgroundcolour = read16();
	chalkcolour = read16();
	aliascolour = read16();

	// read blackboard strings
	for (unsigned int i = 0; i < 48; i++) {
		uint32 value = read32();
		std::string str = readBytes(11);
		// chop off non-null-terminated bits
		str = std::string(str.c_str());
		// TODO: are value keys unique?
		strings[value] = str;
	}
}

void SFCVehicle::read() {
	SFCCompoundObject::read();

	// discard unknown bytes
	readBytes(9);
	read16();
	assert(read16() == 0);
	read16();
	assert(read8() == 0);

	// read cabin boundaries
	cabinleft = read32();
	cabintop = read32();
	cabinright = read32();
	cabinbottom = read32();

	// discard unknown bytes
	assert(read32() == 0);
}

void SFCLift::read() {
	SFCVehicle::read();
	
	// discard unknown bytes
	readBytes(65);
}

void SFCSimpleObject::read() {
	SFCObject::read();

	entity = (SFCEntity *)slurpMFC(TYPE_ENTITY);
}

void SFCPointerTool::read() {
	SFCSimpleObject::read();

	// discard unknown bytes
	readBytes(51);
}

void SFCCallButton::read() {
	SFCSimpleObject::read();

	ourLift = (SFCLift *)slurpMFC(TYPE_LIFT);
	liftid = read8();
}

void SFCScript::read(SFCFile *f) {
	genus = f->read8();
	family = f->read8();
	eventno = f->read16();
	species = f->read16();
	data = f->readstring();
}

// ------------------------------------------------------------------

#include "World.h"

void SFCFile::copyToWorld() {
	mapdata->copyToWorld();

	for (std::vector<SFCObject *>::iterator i = objects.begin(); i != objects.end(); i++) {
		(*i)->copyToWorld();
	}
	
	for (std::vector<SFCScenery *>::iterator i = scenery.begin(); i != scenery.end(); i++) {
		(*i)->copyToWorld();
	}

	for (std::vector<SFCScript>::iterator i = scripts.begin(); i != scripts.end(); i++) {
		i->install();
	}
	
	world.camera.moveTo(scrollx, scrolly, jump);
}

void MapData::copyToWorld() {
	creaturesImage *spr = world.gallery.getImage(background->filename);
	// TODO: hardcoded size bad?
	MetaRoom *m = new MetaRoom(0, 0, 8352, 2400, background->filename, spr);
	world.map.addMetaRoom(m);

	for (std::vector<CRoom *>::iterator i = rooms.begin(); i != rooms.end(); i++) {
		CRoom *src = *i;
		Room *r = new Room(src->left, src->right, src->top, src->top, src->bottom, src->bottom);
		r->type = src->roomtype;
		unsigned int roomid = m->addRoom(r);
		assert(roomid == src->id);

		// TODO: ca values, sources, wind, drop status, music
		// TODO: floor points
		// TODO: floor value
	}
	
	for (std::vector<CRoom *>::iterator i = rooms.begin(); i != rooms.end(); i++) {
		CRoom *src = *i;

		for (unsigned int j = 0; j < 4; j++) {
			for (std::vector<CDoor *>::iterator k = src->doors[j].begin(); k < src->doors[j].end(); k++) {
				CDoor *door = *k;
				Room *r1 = world.map.getRoom(src->id);
				Room *r2 = world.map.getRoom(door->otherroom);
		
				if (r1->doors.find(r2) == r1->doors.end()) {
					// create a new door between rooms!
					RoomDoor *roomdoor = new RoomDoor();
					roomdoor->first = r1;
					roomdoor->second = r2;
					roomdoor->perm = door->openness;
					r1->doors[r2] = roomdoor;
					r2->doors[r1] = roomdoor;
					// TODO: ADDR adds to nearby?
				} else {
					// sanity check
					RoomDoor *roomdoor = r1->doors[r2];
					assert(roomdoor->perm == door->openness);
				}
			}
		}
	}

	// TODO: misc data?
}

void SFCObject::copyToWorld() {
	// TODO: this is a stub, make it pure virtual when we're done implementing
}

#include "SimpleAgent.h"

#include <iostream> // TODO: remove

void SFCSimpleObject::copyToWorld() {
	// construct our equivalent object
	SimpleAgent *a = new SimpleAgent(family, genus, species, entity->zorder, sprite->filename, sprite->firstimg, sprite->noframes);
	a->finishInit();
	//a->moveTo(entity->x - (a->part(0)->getWidth() / 2), entity->y - (a->part(0) -> getHeight() / 2));
	a->moveTo(entity->x, entity->y);
	
	// copy data from ourselves
	
	// C2 attributes are a subset of c2e ones
	if (attr & 128) attr -= 128; // TODO: hack to disable physics, for now
	a->setAttributes(attr);
	
	// TODO: bhvr click state
	// TODO: ticking
	
	for (unsigned int i = 0; i < 100; i++)
		a->var[i].setInt(variables[i]);
	
	a->perm = size; // TODO
	// TODO: threat
	a->range = range;
	a->accg = accg;
	a->velx.setInt(velx);
	a->vely.setInt(vely);
	a->elas = rest; // TODO
	a->aero = aero;
	a->paused = frozen; // TODO

	// copy data from entity
	DullPart *p = (DullPart *)a->part(0);
	
	// pose
	p->setPose(entity->currframe);
	
	// animation
	if (entity->haveanim) {
		for (unsigned int i = 0; i < entity->animstring.size(); i++) {
			if (entity->animstring[i] == 'R')
				p->animation.push_back(255);
			else {
				assert(entity->animstring[i] >= 48 && entity->animstring[i] <= 57);
				p->animation.push_back(entity->animstring[i] - 48);
			}
		}

		// TODO: should make sure animation position == current pose
		if (entity->animframe < p->animation.size()) {
			if (p->animation[entity->animframe] == 255)
				p->setFrameNo(0);
			else
				p->setFrameNo(entity->animframe);
		} else p->animation.clear();
	}

	// TODO: bhvr
	// TODO: pickup handles/points
	// TODO: imgoffset
}

#include <boost/format.hpp>
#include <sstream>

void SFCScript::install() {
	std::string scriptinfo = boost::str(boost::format("<SFC script %d, %d, %d: %d>") % (int)family % (int)genus % species % eventno);
	caosScript script(world.gametype, scriptinfo);
	std::istringstream s(data);
	try {
		script.parse(s);
		script.installInstallScript(family, genus, species, eventno);
		script.installScripts();
	} catch (std::exception &e) {
		std::cerr << "installation of \"" << scriptinfo << "\" failed due to exception " << e.what() << std::endl;
	}
}

/* vim: set noet: */