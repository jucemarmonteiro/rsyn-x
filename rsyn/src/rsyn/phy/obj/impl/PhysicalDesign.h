/* Copyright 2014-2017 Rsyn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

namespace Rsyn {

// -----------------------------------------------------------------------------

inline void PhysicalDesign::loadLibrary(const LefDscp & library) {
	if (!data) {
		std::cout << "ERROR: Physical Design was not configured!\n"
			<< "Please call first initPhysicalDesign!"
			<< std::endl;
		return;
	} // end if 

	if (library.clsLefUnitsDscp.clsHasDatabase) {
		if (getDatabaseUnits(LIBRARY_DBU) == 0) {
			data->clsDBUs[LIBRARY_DBU] = library.clsLefUnitsDscp.clsDatabase;
		} else {
			if (getDatabaseUnits(LIBRARY_DBU) != library.clsLefUnitsDscp.clsDatabase) {
				std::cout << "WARNING: Stored LEF database units "
					<< getDatabaseUnits(LIBRARY_DBU)
					<< " is not equal to LEF database units defined in Library "
					<< library.clsLefUnitsDscp.clsDatabase
					<< ".\n";
				std::cout << "WARNING: Lef library elements were NOT initialized!\n";
				return;
			} // end if 
		} // end if-else
	} // end if 

	// Initializing physical sites
	data->clsPhysicalSites.reserve(library.clsLefSiteDscps.size());
	for (const LefSiteDscp & lefSite : library.clsLefSiteDscps) {
		addPhysicalSite(lefSite);
	} // end for

	// Initializing physical layers
	data->clsPhysicalLayers.reserve(library.clsLefLayerDscps.size());
	for (const LefLayerDscp & lefLayer : library.clsLefLayerDscps) {
		addPhysicalLayer(lefLayer);
	} // end for 

	// Initializing physical vias
	data->clsPhysicalVias.reserve(library.clsLefViaDscps.size());
	for (const LefViaDscp & via : library.clsLefViaDscps) {
		addPhysicalVia(via);
	} // end for 

	// initializing physical spacing 
	for (const LefSpacingDscp & spc : library.clsLefSpacingDscps) {
		addPhysicalSpacing(spc);
	} // end for 

	// initializing physical cells (LEF macros)
	for (const LefMacroDscp & macro : library.clsLefMacroDscps) {
		// Adding Physical Library cell to Physical Layer
		Rsyn::LibraryCell libCell = addPhysicalLibraryCell(macro);

		// Adding Physical Library pins to Physical Layer
		for (const LefPinDscp &lpin : macro.clsPins) {
			addPhysicalLibraryPin(libCell, lpin);
		} // end for	
	} // end for

} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::loadDesign(const DefDscp & design) {
	if (data->clsLoadDesign) {
		std::cout << "WARNING: Design was already loaded. Skipping ... \n";
		return;
	} // end if
	data->clsLoadDesign = true;

	// Adding Library cell to Physical Layer
	data->clsDBUs[DESIGN_DBU] = design.clsDatabaseUnits;
	data->clsPhysicalDie.clsBounds = design.clsDieBounds;

	if (getDatabaseUnits(LIBRARY_DBU) % getDatabaseUnits(DESIGN_DBU) != 0) {
		std::cout << "ERROR: Invalid DEF database units " << getDatabaseUnits(DESIGN_DBU) << " (LEF database units: " << getDatabaseUnits(LIBRARY_DBU) << ").\n";
		std::cout << "DEF design units should be lower or equal to LEF design units and must have a integer multiple. Physical design was not initialized!\n";
		return;
	} // end if 

	// This operation always results in an integer number factor. 
	// LEF/DEF specifications prohibit division that results in a real number factor.
	data->clsDBUs[MULT_FACTOR_DBU] = getDatabaseUnits(LIBRARY_DBU) / getDatabaseUnits(DESIGN_DBU);

	// initializing physical cells (DEF Components)
	for (const DefComponentDscp & component : design.clsComps) {
		// Adding Physical cell to Physical Layer
		Rsyn::Cell cell = data->clsDesign.findCellByName(component.clsName);
		if (!cell) {
			throw Exception("Cell " + component.clsName + " not found.\n");
		} // end if
		addPhysicalCell(cell, component);
	} // end for

	// Initializing circuit ports
	for (const DefPortDscp & pin_port : design.clsPorts) {
		// Adding Library cell to Physical Layer
		Rsyn::Port cell = data->clsDesign.findPortByName(pin_port.clsName);
		if (!cell) {
			throw Exception("Port " + pin_port.clsName + " not found.\n");
		} // end if
		addPhysicalPort(cell, pin_port);
	} // end for

	//Initializing circuit rows and defining circuit bounds
	Rsyn::PhysicalModule phModule = getPhysicalModule(data->clsModule);
	Bounds & bounds = phModule->clsBounds;
	bounds[LOWER].apply(+std::numeric_limits<DBU>::max());
	bounds[UPPER].apply(-std::numeric_limits<DBU>::max());
	data->clsPhysicalRows.reserve(design.clsRows.size()); // reserving space for rows
	for (const DefRowDscp & defRow : design.clsRows) {
		addPhysicalRow(defRow);
	} // end for 

	data->clsMapPhysicalRegions.reserve(design.clsRegions.size());
	data->clsPhysicalRegions.reserve(design.clsRegions.size());
	for (const DefRegionDscp & defRegion : design.clsRegions)
		addPhysicalRegion(defRegion);

	data->clsMapPhysicalGroups.reserve(design.clsGroups.size());
	data->clsPhysicalGroups.reserve(design.clsGroups.size());
	for (const DefGroupDscp & defGroup : design.clsGroups)
		addPhysicalGroup(defGroup);

	for (const DefNetDscp & net : design.clsNets)
		addPhysicalNet(net);

	// only to keep coherence in the design;
	data->clsNumElements[PHYSICAL_PORT] = data->clsDesign.getNumInstances(Rsyn::PORT);
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::initPhysicalDesign(Rsyn::Design dsg, const Json &params) {
	if (data) {
		std::cout << "ERROR: design already set.\nSkipping initialize Physical Design\n";
		return;
	} // end if
	this->data = new PhysicalDesignData();

	if (!params.is_null()) {
		data->clsEnablePhysicalPins = params.value("clsEnablePhysicalPins", data->clsEnablePhysicalPins);
		data->clsEnableMergeRectangles = params.value("clsEnableMergeRectangles", data->clsEnableMergeRectangles);
		data->clsEnableNetPinBoundaries = params.value("clsEnableNetPinBoundaries", data->clsEnableNetPinBoundaries);
	} // end if 

	data->clsDesign = dsg;
	data->clsModule = dsg.getTopModule();
	data->clsPhysicalInstances = dsg.createAttribute();
	data->clsPhysicalLibraryCells = dsg.createAttribute();
	data->clsPhysicalLibraryPins = dsg.createAttribute();
	if (data->clsEnablePhysicalPins)
		data->clsPhysicalPins = dsg.createAttribute();
	data->clsPhysicalNets = dsg.createAttribute();
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::setClockNet(Rsyn::Net net) {
	data->clsClkNet = net;
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::updateAllNetBounds(const bool skipClockNet) {
	if (skipClockNet && data->clsClkNet) {
		Rsyn::PhysicalNet phNet = getPhysicalNet(data->clsClkNet);
		data->clsHPWL -= phNet.getHPWL();
	} // end if 

	for (Rsyn::Net net : data->clsModule.allNets()) {
		//skipping clock network
		if (skipClockNet && (data->clsClkNet == net))
			continue;

		updateNetBound(net);
	} // end for
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::updateNetBound(Rsyn::Net net) {
	// net has not pins. The boundaries are defined by default to 0.
	if (net.getNumPins() == 0)
		return;

	PhysicalNetData &phNet = data->clsPhysicalNets[net];
	Bounds &bound = phNet.clsBounds;
	data->clsHPWL -= bound.computeLength(); // remove old net wirelength
	bound[UPPER].apply(-std::numeric_limits<DBU>::max());
	bound[LOWER].apply(+std::numeric_limits<DBU>::max());
	const bool updatePinBound = data->clsEnableNetPinBoundaries;
	for (Rsyn::Pin pin : net.allPins()) {
		DBUxy pos = getPinPosition(pin);

		// upper x corner pos
		if (pos[X] >= bound[UPPER][X]) {
			bound[UPPER][X] = pos[X];
			if (updatePinBound)
				phNet.clsBoundPins[UPPER][X] = pin;
		} // end if 

		// lower x corner pos 
		if (pos[X] <= bound[LOWER][X]) {
			bound[LOWER][X] = pos[X];
			if (updatePinBound)
				phNet.clsBoundPins[LOWER][X] = pin;
		} // end if 

		// upper y corner pos 
		if (pos[Y] >= bound[UPPER][Y]) {
			bound[UPPER][Y] = pos[Y];
			if (updatePinBound)
				phNet.clsBoundPins[UPPER][Y] = pin;
		} // end if 

		// lower y corner pos 
		if (pos[Y] <= bound[LOWER][Y]) {
			bound[LOWER][Y] = pos[Y];
			if (updatePinBound)
				phNet.clsBoundPins[LOWER][Y] = pin;
		} // end if 
	} // end for
	data->clsHPWL += bound.computeLength(); // update hpwl 
} // end method 

// -----------------------------------------------------------------------------

inline DBU PhysicalDesign::getDatabaseUnits(const DBUType type) const {
	return data->clsDBUs[type];
} // end method  

// -----------------------------------------------------------------------------

inline DBUxy PhysicalDesign::getHPWL() const {
	return data->clsHPWL;
} // end method 

// -----------------------------------------------------------------------------

inline DBU PhysicalDesign::getHPWL(const Dimension dim) const {
	return data->clsHPWL[dim];
}// end method

// -----------------------------------------------------------------------------

inline int PhysicalDesign::getNumElements(PhysicalType type) const {
	return data->clsNumElements[type];
} // end method 

// -----------------------------------------------------------------------------

inline DBU PhysicalDesign::getArea(const PhysicalType type) const {
	return data->clsTotalAreas[type];
} // end method 

// -----------------------------------------------------------------------------

inline bool PhysicalDesign::isEnablePhysicalPins() const {
	return data->clsEnablePhysicalPins;
} // end method 

// -----------------------------------------------------------------------------

inline bool PhysicalDesign::isEnableMergeRectangles() const {
	return data->clsEnableMergeRectangles;
} // end method 

// -----------------------------------------------------------------------------

inline bool PhysicalDesign::isEnableNetPinBoundaries() const {
	return data->clsEnableNetPinBoundaries;
} // end method 

// -----------------------------------------------------------------------------

// Adding the new Site parameter to PhysicalDesign data structure.

inline void PhysicalDesign::addPhysicalSite(const LefSiteDscp & site) {
	std::unordered_map<std::string, int>::iterator it = data->clsMapPhysicalSites.find(site.clsName);
	if (it != data->clsMapPhysicalSites.end()) {
		std::cout << "WARNING: Site " << site.clsName << " was already defined. Skipping ...\n";
		return;
	} // end if 

	// Adding new lib site
	data->clsPhysicalSites.push_back(PhysicalSite(new PhysicalSiteData()));
	PhysicalSite phSite = data->clsPhysicalSites.back();
	data->clsMapPhysicalSites[site.clsName] = data->clsPhysicalSites.size() - 1;
	phSite->id = data->clsPhysicalSites.size() - 1;
	phSite->clsSiteName = site.clsName;
	double2 size = site.clsSize;
	size.scale(static_cast<double> (getDatabaseUnits(LIBRARY_DBU)));
	phSite->clsSize[X] = static_cast<DBU> (std::round(size[X]));
	phSite->clsSize[Y] = static_cast<DBU> (std::round(size[Y]));
	phSite->clsSiteClass = getPhysicalSiteClass(site.clsSiteClass);
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalLayer(const LefLayerDscp& layer) {
	std::unordered_map<std::string, std::size_t>::iterator it = data->clsMapPhysicalLayers.find(layer.clsName);
	if (it != data->clsMapPhysicalLayers.end()) {
		std::cout << "WARNING: Layer " << layer.clsName << " was already defined. Skipping ...\n";
		return;
	} // end if 

	Element<PhysicalLayerData> *element = data->clsPhysicalLayers.create();
	Rsyn::PhysicalLayerData * phLayer = &(element->value);
	phLayer->id = data->clsPhysicalLayers.lastId();
	phLayer->clsName = layer.clsName;
	phLayer->clsDirection = Rsyn::getPhysicalLayerDirection(layer.clsDirection);
	phLayer->clsType = Rsyn::getPhysicalLayerType(layer.clsType);
	phLayer->clsPitch = static_cast<DBU> (std::round(layer.clsPitch * getDatabaseUnits(LIBRARY_DBU)));
	phLayer->clsSpacing = static_cast<DBU> (std::round(layer.clsSpacing * getDatabaseUnits(LIBRARY_DBU)));
	phLayer->clsWidth = static_cast<DBU> (std::round(layer.clsWidth * getDatabaseUnits(LIBRARY_DBU)));
	phLayer->clsRelativeIndex = data->clsNumLayers[phLayer->clsType];
	data->clsMapPhysicalLayers[layer.clsName] = phLayer->id;
	data->clsNumLayers[phLayer->clsType]++;
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalVia(const LefViaDscp & via) {
	std::unordered_map<std::string, std::size_t>::iterator it = data->clsMapPhysicalVias.find(via.clsName);
	if (it != data->clsMapPhysicalVias.end()) {
		std::cout << "WARNING: Site " << via.clsName << " was already defined. Skipping ...\n";
		return;
	} // end if 

	// Adding new lib site
	data->clsMapPhysicalVias[via.clsName] = data->clsPhysicalVias.size();
	data->clsPhysicalVias.push_back(PhysicalVia(new PhysicalViaData()));
	PhysicalVia phVia = data->clsPhysicalVias.back();
	phVia->clsName = via.clsName;
	phVia->clsViaLayers.reserve(via.clsViaLayers.size());
	for (const LefViaLayerDscp & layerDscp : via.clsViaLayers) {
		phVia->clsViaLayers.push_back(PhysicalViaLayer(new PhysicalViaLayerData()));
		PhysicalViaLayer phViaLayer = phVia->clsViaLayers.back();
		phViaLayer->clsLayer = getPhysicalLayerByName(layerDscp.clsLayerName);
		phViaLayer->clsBounds.reserve(layerDscp.clsBounds.size());
		for (DoubleRectangle doubleRect : layerDscp.clsBounds) {
			doubleRect.scaleCoordinates(static_cast<double> (getDatabaseUnits(LIBRARY_DBU)));
			phViaLayer->clsBounds.push_back(Bounds());
			Bounds & bds = phViaLayer->clsBounds.back();
			bds[LOWER][X] = static_cast<DBU> (std::round(doubleRect[LOWER][X]));
			bds[LOWER][Y] = static_cast<DBU> (std::round(doubleRect[LOWER][Y]));
			bds[UPPER][X] = static_cast<DBU> (std::round(doubleRect[UPPER][X]));
			bds[UPPER][Y] = static_cast<DBU> (std::round(doubleRect[UPPER][Y]));
		} // end for 
	} // end for
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::LibraryCell PhysicalDesign::addPhysicalLibraryCell(const LefMacroDscp& macro) {
	Rsyn::LibraryCell lCell = data->clsDesign.findLibraryCellByName(macro.clsMacroName);
	if (!lCell) {
		throw Exception("Library Cell " + macro.clsMacroName + " not found.\n");
	} // end if
	Rsyn::PhysicalLibraryCellData &phlCell = data->clsPhysicalLibraryCells[lCell];
	double2 size = macro.clsSize;
	size.scale(static_cast<double> (getDatabaseUnits(LIBRARY_DBU)));
	phlCell.clsSize[X] = static_cast<DBU> (std::round(size[X]));
	phlCell.clsSize[Y] = static_cast<DBU> (std::round(size[Y]));
	phlCell.clsMacroClass = Rsyn::getPhysicalMacroClass(macro.clsMacroClass);
	// Initializing obstacles in the physical library cell
	phlCell.clsObs.reserve(macro.clsObs.size());
	for (const LefObsDscp &libObs : macro.clsObs) {
		PhysicalObstacle phObs(new PhysicalObstacleData());

		// Guilherme Flach - 2016/11/04 - micron to dbu
		std::vector<Bounds> scaledBounds;
		scaledBounds.reserve(libObs.clsBounds.size());
		for (DoubleRectangle doubleRect : libObs.clsBounds) {
			// Jucemar Monteiro - 2017/05/20 - Avoiding cast round errors.
			doubleRect.scaleCoordinates(static_cast<double> (getDatabaseUnits(LIBRARY_DBU)));
			scaledBounds.push_back(Bounds());
			Bounds & bds = scaledBounds.back();
			bds[LOWER][X] = static_cast<DBU> (std::round(doubleRect[LOWER][X]));
			bds[LOWER][Y] = static_cast<DBU> (std::round(doubleRect[LOWER][Y]));
			bds[UPPER][X] = static_cast<DBU> (std::round(doubleRect[UPPER][X]));
			bds[UPPER][Y] = static_cast<DBU> (std::round(doubleRect[UPPER][Y]));
		} // end for

		phlCell.clsObs.push_back(phObs);
		if (data->clsEnableMergeRectangles && libObs.clsBounds.size() > 1) {
			std::vector<Bounds> bounds;
			mergeBounds(scaledBounds, bounds);
			phObs->clsBounds.reserve(bounds.size());
			for (Bounds & rect : bounds) {
				//rect.scaleCoordinates(data->clsDesignUnits);
				phObs->clsBounds.push_back(rect);
			} // end for 
		} else {
			phObs->clsBounds = scaledBounds;
		} // end if-else 
		phObs->clsLayer = getPhysicalLayerByName(libObs.clsMetalLayer);
		phObs->id = phlCell.clsObs.size() - 1;
		// Hard code. After implementing mapping structure in Rsyn, remove this line.
		if (libObs.clsMetalLayer.compare("metal1") == 0) {
			phlCell.clsLayerBoundIndex = phlCell.clsObs.size() - 1;
		}
	} // end for
	return lCell;
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalLibraryPin(Rsyn::LibraryCell libCell, const LefPinDscp& lefPin) {
	Rsyn::LibraryPin rsynLibraryPin = libCell.getLibraryPinByName(lefPin.clsPinName);
	if (!rsynLibraryPin) {
		throw Exception("Pin '" + lefPin.clsPinName + "' not found in library cell '" + libCell.getName() + "'.\n");
	} // end if
	Rsyn::PhysicalLibraryPinData &phyPin = data->clsPhysicalLibraryPins[rsynLibraryPin];

	phyPin.clsDirection = Rsyn::getPhysicalPinDirection(lefPin.clsPinDirection);
	//Assuming that the pin has only one port.
	phyPin.clsPhysicalPinPorts.reserve(lefPin.clsPorts.size());
	for (const LefPortDscp & lefPort : lefPin.clsPorts) {
		phyPin.clsPhysicalPinPorts.push_back(PhysicalPinPort(new PhysicalPinPortData()));
		PhysicalPinPort phPinPort = phyPin.clsPhysicalPinPorts.back();
		phPinPort->id = phyPin.clsPhysicalPinPorts.size() - 1;

		PhysicalPinLayer phPinLayer(new PhysicalPinLayerData());
		phPinLayer->id = 0;
		phPinLayer->clsLibLayer = getPhysicalLayerByName(lefPort.clsMetalName);
		phPinLayer->clsBounds.reserve(lefPort.clsBounds.size());
		phPinPort->clsLayer = phPinLayer;
		//ICCAD 2015 contest pin bounds definition
		if (!lefPort.clsBounds.empty()) {
			phyPin.clsLayerBound[LOWER] = DBUxy(std::numeric_limits<DBU>::max(), std::numeric_limits<DBU>::max());
			phyPin.clsLayerBound[UPPER] = DBUxy(std::numeric_limits<DBU>::min(), std::numeric_limits<DBU>::min());
			for (DoubleRectangle doubleRect : lefPort.clsBounds) {
				doubleRect.scaleCoordinates(static_cast<double> (getDatabaseUnits(LIBRARY_DBU)));
				phPinLayer->clsBounds.push_back(Bounds());
				Bounds & bds = phPinLayer->clsBounds.back();
				bds[LOWER][X] = static_cast<DBU> (std::round(doubleRect[LOWER][X]));
				bds[LOWER][Y] = static_cast<DBU> (std::round(doubleRect[LOWER][Y]));
				bds[UPPER][X] = static_cast<DBU> (std::round(doubleRect[UPPER][X]));
				bds[UPPER][Y] = static_cast<DBU> (std::round(doubleRect[UPPER][Y]));

				phyPin.clsLayerBound[LOWER][X] = std::min(phyPin.clsLayerBound[LOWER][X], bds[LOWER][X]);
				phyPin.clsLayerBound[LOWER][Y] = std::min(phyPin.clsLayerBound[LOWER][Y], bds[LOWER][Y]);
				phyPin.clsLayerBound[UPPER][X] = std::max(phyPin.clsLayerBound[UPPER][X], bds[UPPER][X]);
				phyPin.clsLayerBound[UPPER][Y] = std::max(phyPin.clsLayerBound[UPPER][Y], bds[UPPER][Y]);
			} // end for
		} // end if 
		if (!lefPort.clsLefPolygonDscp.empty()) {
			phyPin.clsLayerBound[LOWER] = DBUxy(std::numeric_limits<DBU>::max(), std::numeric_limits<DBU>::max());
			phyPin.clsLayerBound[UPPER] = DBUxy(std::numeric_limits<DBU>::min(), std::numeric_limits<DBU>::min());
			phPinLayer->clsPolygons.reserve(lefPort.clsLefPolygonDscp.size());
			for (const LefPolygonDscp & poly : lefPort.clsLefPolygonDscp) {
				phPinLayer->clsPolygons.push_back(Polygon());
				Polygon &phPoly = phPinLayer->clsPolygons.back();
				phPoly.reserve(poly.clsPolygonPoints.size());
				for (double2 point : poly.clsPolygonPoints) {
					point.scale(static_cast<double> (getDatabaseUnits(LIBRARY_DBU)));
					DBUxy pt(static_cast<DBU> (std::round(pt[X])), static_cast<DBU> (std::round(pt[Y])));
					phPoly.addPoint(pt);
					phyPin.clsLayerBound[LOWER][X] = std::min(phyPin.clsLayerBound[LOWER][X], pt[X]);
					phyPin.clsLayerBound[LOWER][Y] = std::min(phyPin.clsLayerBound[LOWER][Y], pt[Y]);
					phyPin.clsLayerBound[UPPER][X] = std::max(phyPin.clsLayerBound[UPPER][X], pt[X]);
					phyPin.clsLayerBound[UPPER][Y] = std::max(phyPin.clsLayerBound[UPPER][Y], pt[Y]);
				} // end for
			} // end for 
		} // end if 
	} // end for 
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalCell(Rsyn::Instance cell, const DefComponentDscp& component) {
	PhysicalLibraryCellData &phLibCell = data->clsPhysicalLibraryCells[cell.asCell().getLibraryCell()]; // TODO: assuming instance is a cell

	PhysicalInstanceData & physicalCell = data->clsPhysicalInstances[cell];
	physicalCell.clsInstance = cell;
	Rsyn::InstanceTag tag = data->clsDesign.getTag(cell);
	tag.setFixed(component.clsIsFixed);
	tag.setMacroBlock(phLibCell.clsMacroClass == Rsyn::MACRO_BLOCK);
	if (component.clsIsFixed) {
		if (phLibCell.clsLayerBoundIndex > -1) {
			PhysicalObstacle obs = phLibCell.clsObs[phLibCell.clsLayerBoundIndex];
			data->clsNumElements[PHYSICAL_FIXEDBOUNDS] += obs->clsBounds.size();
		} else {
			data->clsNumElements[PHYSICAL_FIXEDBOUNDS]++;
		} // end if-else
		data->clsNumElements[PHYSICAL_FIXED]++;
	} // end if 
	physicalCell.clsPlaced = component.clsIsPlaced;
	physicalCell.clsBlock = phLibCell.clsMacroClass == Rsyn::MACRO_BLOCK;
	if (physicalCell.clsBlock)
		data->clsNumElements[PHYSICAL_BLOCK]++;
	if (cell.isMovable())
		data->clsNumElements[PHYSICAL_MOVABLE]++;
	physicalCell.clsHasLayerBounds = phLibCell.clsLayerBoundIndex > -1;
	physicalCell.clsPhysicalOrientation = getPhysicalOrientation(component.clsOrientation);

	const DBU width = phLibCell.clsSize[X];
	const DBU height = phLibCell.clsSize[Y];

	DBUxy pos = component.clsPos;
	physicalCell.clsInitialPos = pos;
	pos[X] += width;
	pos[Y] += height;
	physicalCell.clsBounds.updatePoints(physicalCell.clsInitialPos, pos);

	DBU area = width * height;
	if (physicalCell.clsBlock)
		data->clsTotalAreas[PHYSICAL_BLOCK] += area;
	if (cell.isFixed()) {
		if (cell.isPort())
			data->clsTotalAreas[PHYSICAL_PORT] += area;
		else
			data->clsTotalAreas[PHYSICAL_FIXED] += area;
	} else {
		data->clsTotalAreas[PHYSICAL_MOVABLE] += area;
	} // end if-else 
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalPort(Rsyn::Instance cell, const DefPortDscp& port) {
	PhysicalInstanceData & physicalGate = data->clsPhysicalInstances[cell];
	physicalGate.clsBounds = Bounds(port.clsICCADPos, port.clsICCADPos);
	physicalGate.clsInstance = cell;
	physicalGate.clsPortLayer = getPhysicalLayerByName(port.clsLayerName);
	Rsyn::InstanceTag tag = data->clsDesign.getTag(cell);
	// TODO Getting from port descriptor if it is fixed.
	tag.setFixed(true);
	tag.setMacroBlock(false);
} // end method  

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalRow(const DefRowDscp& defRow) {
	PhysicalSite phSite = getPhysicalSiteByName(defRow.clsSite);
	if (!phSite)
		throw Exception("Site " + defRow.clsSite + " was not find for row " + defRow.clsName);

	// Creates a new cell in the data structure.
	PhysicalRowData * phRowData = &(data->clsPhysicalRows.create()->value); // TODO: awful
	phRowData->id = data->clsPhysicalRows.lastId();
	phRowData->clsRowName = defRow.clsName;
	phRowData->clsPhysicalSite = phSite;
	phRowData->clsOrigin = defRow.clsOrigin;
	phRowData->clsStep[X] = phSite.getWidth();
	phRowData->clsStep[Y] = phSite.getHeight();
	phRowData->clsNumSites[X] = defRow.clsNumX;
	phRowData->clsNumSites[Y] = defRow.clsNumY;
	phRowData->clsBounds[LOWER][X] = defRow.clsOrigin[X];
	phRowData->clsBounds[LOWER][Y] = defRow.clsOrigin[Y];
	phRowData->clsBounds[UPPER][X] = defRow.clsOrigin[X] + phRowData->getWidth();
	phRowData->clsBounds[UPPER][Y] = defRow.clsOrigin[Y] + phRowData->getHeight();
	Rsyn::PhysicalModule phModule = getPhysicalModule(data->clsModule);
	Bounds & bounds = phModule->clsBounds;
	bounds[LOWER] = min(bounds[LOWER], phRowData->clsBounds[LOWER]);
	bounds[UPPER] = max(bounds[UPPER], phRowData->clsBounds[UPPER]);
	data->clsTotalAreas[PHYSICAL_PLACEABLE] += phRowData->clsBounds.computeArea();
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalRegion(const DefRegionDscp& defRegion) {
	data->clsPhysicalRegions.push_back(PhysicalRegion(new PhysicalRegionData()));
	Rsyn::PhysicalRegion phRegion = data->clsPhysicalRegions.back();
	phRegion->id = data->clsPhysicalRegions.size() - 1;
	phRegion->clsName = defRegion.clsName;
	phRegion->clsType = Rsyn::getPhysicalRegionType(defRegion.clsType);
	phRegion->clsBounds = defRegion.clsBounds;
	data->clsMapPhysicalRegions[defRegion.clsName] = phRegion->id;
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalGroup(const DefGroupDscp& defGroup) {
	data->clsPhysicalGroups.push_back(PhysicalGroup(new PhysicalGroupData()));
	Rsyn::PhysicalGroup phGroup = data->clsPhysicalGroups.back();
	phGroup->id = data->clsPhysicalGroups.size() - 1;
	phGroup->clsName = defGroup.clsName;
	phGroup->clsPatterns = defGroup.clsPatterns;
	phGroup->clsRegion = getPhysicalRegionByName(defGroup.clsRegion);
	data->clsMapPhysicalGroups[defGroup.clsName] = phGroup->id;
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalNet(const DefNetDscp & netDscp) {
	Rsyn::Net net = data->clsDesign.findNetByName(netDscp.clsName);
	PhysicalNetData & netData = data->clsPhysicalNets[net];
	netData.clsWires.reserve(netDscp.clsWires.size());
	if (netDscp.clsWires.size() > 0)
		netData.clsViaInstances.reserve(100);
	for (const DefWireDscp & wireDscp : netDscp.clsWires) {
		netData.clsWires.push_back(PhysicalWire(new PhysicalWireData()));
		PhysicalWire phWire = netData.clsWires.back();
		phWire->clsWireSegments.reserve(wireDscp.clsWireSegments.size());
		for (const DefWireSegmentDscp & segmentDscp : wireDscp.clsWireSegments) {
			phWire->clsWireSegments.push_back(PhysicalWireSegment(new PhysicalWireSegmentData()));
			PhysicalWireSegment phWireSegment = phWire->clsWireSegments.back();
			phWireSegment->clsPoints = segmentDscp.clsPoints;
			phWireSegment->clsNew = segmentDscp.clsNew;
			phWireSegment->clsPhysicalLayer = getPhysicalLayerByName(segmentDscp.clsLayerName);
			if (segmentDscp.clsHasVia) {
				phWireSegment->clsPhysicalVia = getPhysicalViaByName(segmentDscp.clsViaName);
				PhysicalViaInstance phViaInst = PhysicalViaInstance(new PhysicalViaInstanceData());
				phViaInst->clsPhysicalVia = phWireSegment->clsPhysicalVia;
				phViaInst->clsPos = phWireSegment->clsPoints.back();
				netData.clsViaInstances.push_back(phViaInst);
			} // end if 
			if (segmentDscp.clsHasRectangle) {
				phWireSegment->clsHasRectangle = true;
				phWireSegment->clsRectangle = segmentDscp.clsRect;
			} // end if

			// Adjust path point according to extensions and direction.
			if (segmentDscp.clsPoints.size() >= 2) {
				const DBU extensionBegin = segmentDscp.clsExtensionBegin >= 0 ?
					segmentDscp.clsExtensionBegin : phWireSegment->clsPhysicalLayer->clsWidth / 2; // TODO: non-default rule
				if (extensionBegin > 0) {
					const int x0 = segmentDscp.clsPoints[0].x;
					const int y0 = segmentDscp.clsPoints[0].y;
					const int x1 = segmentDscp.clsPoints[1].x;
					const int y1 = segmentDscp.clsPoints[1].y;

					const int dx = x0 == x1 ? 0 : (x1 > x0 ? -1 : +1);
					const int dy = y0 == y1 ? 0 : (y1 > y0 ? -1 : +1);
					phWireSegment->clsPoints[0].x += dx*extensionBegin;
					phWireSegment->clsPoints[0].y += dy*extensionBegin;
				} // end if

				const DBU extensionEnd = segmentDscp.clsExtensionEnd >= 0 ?
					segmentDscp.clsExtensionEnd : phWireSegment->clsPhysicalLayer->clsWidth / 2; // TODO: non-default rule
				if (extensionEnd > 0) {
					const int k = (int) segmentDscp.clsPoints.size() - 1;

					const int x0 = segmentDscp.clsPoints[k - 1].x;
					const int y0 = segmentDscp.clsPoints[k - 1].y;
					const int x1 = segmentDscp.clsPoints[k].x;
					const int y1 = segmentDscp.clsPoints[k].y;

					const int dx = x0 == x1 ? 0 : (x1 > x0 ? +1 : -1);
					const int dy = y0 == y1 ? 0 : (y1 > y0 ? +1 : -1);
					phWireSegment->clsPoints[k].x += dx*extensionEnd;
					phWireSegment->clsPoints[k].y += dy*extensionEnd;
				} // end if
			} // end if
		} // end for 
	} // end for
	if (netDscp.clsWires.size() > 0)
		netData.clsViaInstances.shrink_to_fit();
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalSpacing(const LefSpacingDscp & spacing) {
	Element<PhysicalSpacingData> *element = data->clsPhysicalSpacing.create();
	Rsyn::PhysicalSpacingData * phSpacing = &(element->value);
	phSpacing->id = data->clsPhysicalSpacing.lastId();
	phSpacing->clsLayer1 = getPhysicalLayerByName(spacing.clsLayer1);
	phSpacing->clsLayer2 = getPhysicalLayerByName(spacing.clsLayer2);
	phSpacing->clsDistance = static_cast<DBU> (std::round(spacing.clsDistance * getDatabaseUnits(LIBRARY_DBU)));
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::addPhysicalPin() {
	std::cout << "TODO " << __func__ << "\n";
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalLayer PhysicalDesign::getPhysicalLayerByName(const std::string & layerName) {
	std::unordered_map<std::string, std::size_t>::iterator element = data->clsMapPhysicalLayers.find(layerName);
	if (element == data->clsMapPhysicalLayers.end())
		return nullptr;
	const int id = element->second;
	Element<PhysicalLayerData> * phLayerDataElement = data->clsPhysicalLayers.get(id);
	return PhysicalLayer(&phLayerDataElement->value);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalSite PhysicalDesign::getPhysicalSiteByName(const std::string & siteName) {
	std::unordered_map<std::string, int>::iterator it = data->clsMapPhysicalSites.find(siteName);
	return it != data->clsMapPhysicalSites.end() ? data->clsPhysicalSites[it->second] : nullptr;
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalRegion PhysicalDesign::getPhysicalRegionByName(const std::string &siteName) {
	std::unordered_map<std::string, std::size_t>::iterator it = data->clsMapPhysicalRegions.find(siteName);
	if (it == data->clsMapPhysicalRegions.end())
		return nullptr;
	return data->clsPhysicalRegions[it->second];
} // end method 

// -----------------------------------------------------------------------------	

inline Rsyn::PhysicalGroup PhysicalDesign::getPhysicalGroupByName(const std::string &siteName) {
	std::unordered_map<std::string, std::size_t>::iterator it = data->clsMapPhysicalGroups.find(siteName);
	if (it == data->clsMapPhysicalGroups.end())
		return nullptr;
	return data->clsPhysicalGroups[it->second];
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalVia PhysicalDesign::getPhysicalViaByName(const std::string &viaName) {
	std::unordered_map<std::string, std::size_t>::iterator it = data->clsMapPhysicalVias.find(viaName);
	if (it == data->clsMapPhysicalVias.end())
		return nullptr;
	return data->clsPhysicalVias[it->second];
} // end method 

// -----------------------------------------------------------------------------

inline int PhysicalDesign::getNumLayers(const Rsyn::PhysicalLayerType type) const {
	return data->clsNumLayers[type];
} // end method 

// -----------------------------------------------------------------------------

inline Range<ListCollection<PhysicalLayerData, PhysicalLayer>>
PhysicalDesign::allPhysicalLayers() {
	return ListCollection<PhysicalLayerData, PhysicalLayer>(data->clsPhysicalLayers);
} // end method

inline std::size_t PhysicalDesign::getNumPhysicalVias() const {
	return data->clsPhysicalVias.size();
} // end method 

// -----------------------------------------------------------------------------

inline const std::vector<PhysicalVia> & PhysicalDesign::allPhysicalVias() const {
	return data->clsPhysicalVias;
} // end method 

// -----------------------------------------------------------------------------

inline std::size_t PhysicalDesign::getNumPhysicalSpacing() const {
	return data->clsPhysicalSpacing.size();
} // end method 

// -----------------------------------------------------------------------------

inline Range<ListCollection<PhysicalSpacingData, PhysicalSpacing>>
PhysicalDesign::allPhysicalSpacing() const {
	return ListCollection<PhysicalSpacingData, PhysicalSpacing>(data->clsPhysicalSpacing);
} // end method 

// -----------------------------------------------------------------------------

inline std::size_t PhysicalDesign::getNumPhysicalRegions() const {
	return data->clsPhysicalRegions.size();
} // end method 

// -----------------------------------------------------------------------------

inline std::vector<PhysicalRegion> & PhysicalDesign::allPhysicalRegions() const {
	return data->clsPhysicalRegions;
} // end method 

// -----------------------------------------------------------------------------

inline std::size_t PhysicalDesign::getNumPhysicalGroups() const {
	return data->clsPhysicalGroups.size();
} // end method 

// -----------------------------------------------------------------------------

inline std::vector<PhysicalGroup> & PhysicalDesign::allPhysicalGroups() const {
	return data->clsPhysicalGroups;
} // end method 

// -----------------------------------------------------------------------------

//I'm assuming all rows have the same height.

inline DBU PhysicalDesign::getRowHeight() const {
	return data->clsPhysicalRows.get(0)->value.getHeight();
} // end method 

// -----------------------------------------------------------------------------

inline DBU PhysicalDesign::getRowSiteWidth() const {
	return data->clsPhysicalRows.get(0)->value.clsPhysicalSite.getWidth();
} // end method 

// -----------------------------------------------------------------------------

inline std::size_t PhysicalDesign::getNumRows() const {
	return data->clsPhysicalRows.size();
} // end method 

// -----------------------------------------------------------------------------

inline Range<ListCollection<PhysicalRowData, PhysicalRow>>
PhysicalDesign::allPhysicalRows() {
	return ListCollection<PhysicalRowData, PhysicalRow>(data->clsPhysicalRows);
} // end method

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalLibraryPin PhysicalDesign::getPhysicalLibraryPin(Rsyn::LibraryPin libPin) const {
	return PhysicalLibraryPin(&data->clsPhysicalLibraryPins[libPin]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalLibraryPin PhysicalDesign::getPhysicalLibraryPin(Rsyn::Pin pin) const {
	if (pin.getInstanceType() != Rsyn::CELL)
		return nullptr;
	return PhysicalLibraryPin(&data->clsPhysicalLibraryPins[pin.getLibraryPin()]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalLibraryCell PhysicalDesign::getPhysicalLibraryCell(Rsyn::Cell cell) const {
	Rsyn::LibraryCell libCell = cell.getLibraryCell();
	return PhysicalLibraryCell(&data->clsPhysicalLibraryCells[libCell]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalLibraryCell PhysicalDesign::getPhysicalLibraryCell(Rsyn::LibraryCell libCell) const {
	return PhysicalLibraryCell(&data->clsPhysicalLibraryCells[libCell]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalCell PhysicalDesign::getPhysicalCell(Rsyn::Cell cell) const {
	return PhysicalCell(&data->clsPhysicalInstances[cell]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalCell PhysicalDesign::getPhysicalCell(Rsyn::Pin pin) const {
	Rsyn::Instance instance = pin.getInstance();
	return getPhysicalCell(instance.asCell());
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalInstance PhysicalDesign::getPhysicalInstance(Rsyn::Instance instance) const {
	return PhysicalInstance(&data->clsPhysicalInstances[instance]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalInstance PhysicalDesign::getPhysicalInstance(Rsyn::Pin pin) const {
	return getPhysicalInstance(pin.getInstance());
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalPort PhysicalDesign::getPhysicalPort(Rsyn::Port port) const {
	return PhysicalPort(&data->clsPhysicalInstances[port]);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalPort PhysicalDesign::getPhysicalPort(Rsyn::Pin pin) const {
	Rsyn::Instance instance = pin.getInstance();
	return getPhysicalPort(instance.asPort());
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalModule PhysicalDesign::getPhysicalModule(Rsyn::Module module) const {
	return PhysicalModule(&data->clsPhysicalInstances[module]);
} // end method 

// -----------------------------------------------------------------------------

inline int PhysicalDesign::getNumMovedCells() const {
	int count = 0;
	for (Rsyn::Instance instance : data->clsModule.allInstances()) {
		if (instance.getType() != Rsyn::CELL)
			continue;
		Rsyn::Cell cell = instance.asCell(); // TODO: hack, assuming that the instance is a cell
		Rsyn::PhysicalCell phCell = getPhysicalCell(cell);
		if (instance.isFixed() || instance.isMacroBlock())
			continue;

		const DBUxy initialPos = phCell.getInitialPosition();
		const DBUxy currentPos = phCell.getPosition();
		if (initialPos[X] != currentPos[X] || initialPos[Y] != initialPos[Y])
			count++;
	} // end for
	return count;
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalModule PhysicalDesign::getPhysicalModule(Rsyn::Pin pin) const {
	Rsyn::Instance instance = pin.getInstance();
	return getPhysicalModule(instance.asModule());
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::updatePhysicalCell(Rsyn::Cell cell) {
	PhysicalInstanceData & phCell = data->clsPhysicalInstances[cell.asCell()];
	PhysicalLibraryCellData &phLibCell = data->clsPhysicalLibraryCells[cell.asCell().getLibraryCell()]; // TODO: assuming instance is a cell

	const DBU width = phLibCell.clsSize[X];
	const DBU height = phLibCell.clsSize[Y];
	const DBUxy pos = phCell.clsBounds[LOWER];

	DBU area = (width * height) - phCell.clsBounds.computeArea();
	if (phLibCell.clsMacroClass == Rsyn::MACRO_BLOCK)
		data->clsTotalAreas[PHYSICAL_BLOCK] += area;
	if (cell.isFixed()) {
		if (cell.isPort())
			data->clsTotalAreas[PHYSICAL_PORT] += area;
		else
			data->clsTotalAreas[PHYSICAL_FIXED] += area;
	} else {
		data->clsTotalAreas[PHYSICAL_MOVABLE] += area;
	} // end if-else 

	phCell.clsBounds.updatePoints(pos, DBUxy(pos[X] + width, pos[Y] + height));
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::removePhysicalCell(Rsyn::Cell cell) {
	PhysicalInstanceData & physicalCell = data->clsPhysicalInstances[cell.asCell()];
	DBU area = physicalCell.clsBounds.computeArea();
	if (physicalCell.clsBlock)
		data->clsTotalAreas[PHYSICAL_BLOCK] -= area;
	if (cell.isFixed()) {
		if (cell.isPort())
			data->clsTotalAreas[PHYSICAL_PORT] -= area;
		else
			data->clsTotalAreas[PHYSICAL_FIXED] -= area;
	} else {
		data->clsTotalAreas[PHYSICAL_MOVABLE] -= area;
	} // end if-else 
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalDie PhysicalDesign::getPhysicalDie() const {
	return PhysicalDie(&data->clsPhysicalDie);
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalPin PhysicalDesign::getPhysicalPin(Rsyn::Pin pin) const {
	if (!data->clsEnablePhysicalPins)
		return nullptr;
	return PhysicalPin(&data->clsPhysicalPins[pin]);
} // end method 

// -----------------------------------------------------------------------------

inline DBUxy PhysicalDesign::getPinDisplacement(Rsyn::Pin pin) const {
	Rsyn::Instance inst = pin.getInstance();
	if (inst.getType() == Rsyn::CELL) {
		Rsyn::LibraryPin libPin = pin.getLibraryPin();
		DBUxy displacement = data->clsPhysicalLibraryPins[libPin].clsLayerBound.computeCenter();
		return displacement;
	} // end if 
	return DBUxy(0, 0);
} // end method 

// -----------------------------------------------------------------------------

inline DBUxy PhysicalDesign::getPinPosition(Rsyn::Pin pin) const {
	// Position may be defined if the instance has info. 
	// I'm assuming the instance doesn't know what is its position. 
	DBUxy pos;
	Rsyn::InstanceType type = pin.getInstanceType();
	switch (type) {
		case Rsyn::CELL:
			pos = getPhysicalCell(pin).getPosition();
			break;
		case Rsyn::MODULE:
			pos = getPhysicalModule(pin).getPosition();
			break;
		case Rsyn::PORT:
			pos = getPhysicalPort(pin).getPosition();
			break;
		default:
			pos.apply(std::numeric_limits<DBU>::infinity());
			std::cout << "WARNING: Position for " << pin.getFullName() << " was not defined for the instance type\n";
	} // end switch 
	return pos + getPinDisplacement(pin);
} // end method 

// -----------------------------------------------------------------------------

// For pins of standard-cells, returns the cell position. For macro-blocks,
// returns the pin position itself.

inline DBUxy PhysicalDesign::getRelaxedPinPosition(Rsyn::Pin pin) const {
	// Position may be defined if the instance has info. 
	// I'm assuming the instance doesn't know what is its position. 
	DBUxy pos;
	Rsyn::InstanceType type = pin.getInstanceType();
	Rsyn::PhysicalCell phCell;
	switch (type) {
		case Rsyn::CELL:
			phCell = getPhysicalCell(pin);
			pos = phCell.getPosition();
			if (pin.isMacroBlockPin())
				pos += getPinDisplacement(pin);
			break;
		case Rsyn::MODULE:
			pos = getPhysicalModule(pin).getPosition();
			pos += getPinDisplacement(pin);
			break;
		case Rsyn::PORT:
			pos = getPhysicalPort(pin).getPosition();
			pos += getPinDisplacement(pin);
			break;
		default:
			pos.apply(std::numeric_limits<DBU>::infinity());
			std::cout << "WARNING: Position for " << pin.getFullName() << " was not defined for the instance type\n";
	} // end switch 
	return pos;
} // end method

// -----------------------------------------------------------------------------

inline DBU PhysicalDesign::getPinDisplacement(Rsyn::Pin pin, const Dimension dim) const {
	DBUxy disp = getPinDisplacement(pin);
	return disp[dim];
} // end method 

// -----------------------------------------------------------------------------

inline DBU PhysicalDesign::getPinPosition(Rsyn::Pin pin, const Dimension dim) const {
	DBUxy pos = getPinPosition(pin);
	return pos[dim];
} // end method 

// -----------------------------------------------------------------------------

inline Rsyn::PhysicalNet PhysicalDesign::getPhysicalNet(Rsyn::Net net) const {
	return PhysicalNet(&data->clsPhysicalNets[net]);
} // end method 

// -----------------------------------------------------------------------------

inline void PhysicalDesign::mergeBounds(const std::vector<Bounds> & source,
	std::vector<Bounds> & target, const Dimension dim) {

	target.reserve(source.size());
	std::set<DBU> stripes;
	const Dimension reverse = REVERSE_DIMENSION[dim];
	DBUxy lower, upper;
	lower[reverse] = +std::numeric_limits<DBU>::max();
	upper[reverse] = -std::numeric_limits<DBU>::max();

	for (const Bounds & bound : source) {
		stripes.insert(bound[LOWER][dim]);
		stripes.insert(bound[UPPER][dim]);
		lower[reverse] = std::min(bound[LOWER][reverse], lower[reverse]);
		upper[reverse] = std::max(bound[UPPER][reverse], upper[reverse]);
	} // end for 

	lower[dim] = *stripes.begin();
	stripes.erase(0);
	for (DBU val : stripes) {
		upper[dim] = val;
		Bounds stripe(lower, upper);
		DBU low, upp;
		bool firstMatch = true;
		for (const Bounds & rect : source) {
			if (!rect.overlap(stripe)) {
				if (!firstMatch) {
					Bounds merged = stripe;
					merged[LOWER][reverse] = low;
					merged[UPPER][reverse] = upp;
					target.push_back(merged);
					firstMatch = true;
				} // end if 
				continue;
			} // end if 
			if (firstMatch) {
				low = rect[LOWER][reverse];
				upp = rect[UPPER][reverse];
				firstMatch = false;
			} else {
				if (upp == rect[LOWER][reverse]) {
					upp = rect[UPPER][reverse];
				} // end if 
			} // end if-else 
		} // end for 
		if (!firstMatch) {
			Bounds merged = stripe;
			merged[LOWER][reverse] = low;
			merged[UPPER][reverse] = upp;
			target.push_back(merged);
			firstMatch = true;
		} // end if 
		lower[dim] = val;
	} // end for 
} // end method 

// -----------------------------------------------------------------------------

inline PhysicalIndex PhysicalDesign::getId(Rsyn::PhysicalRow phRow) const {
	return phRow->id;
} // end method 

// -----------------------------------------------------------------------------

inline PhysicalIndex PhysicalDesign::getId(Rsyn::PhysicalLayer phLayer) const {
	return phLayer->id;
} // end method 

// -----------------------------------------------------------------------------

inline PhysicalIndex PhysicalDesign::getId(Rsyn::PhysicalSpacing spacing) const {
	return spacing->id;
} // end method

// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////

inline PhysicalAttributeInitializer PhysicalDesign::createPhysicalAttribute() {
	return PhysicalAttributeInitializer(*this);
} // end method

// -----------------------------------------------------------------------------

template<typename DefaultPhysicalValueType>
inline PhysicalAttributeInitializerWithDefaultValue<DefaultPhysicalValueType>
PhysicalDesign::createPhysicalAttribute(const DefaultPhysicalValueType &defaultValue) {
	return PhysicalAttributeInitializerWithDefaultValue<DefaultPhysicalValueType>(*this, defaultValue);
} // end method


// -----------------------------------------------------------------------------

// Caution when using dontNotifyObservers.
// We can use it when you may expect the move to be rolled back, but it is
// not, recall to mark the cell as dirty.

inline void PhysicalDesign::placeCell(Rsyn::PhysicalCell physicalCell, const DBU x, const DBU y, const bool dontNotifyObservers) {
	const double preivousX = physicalCell.getCoordinate(LOWER, X);
	const double preivousY = physicalCell.getCoordinate(LOWER, Y);

	physicalCell->clsBounds.moveTo(x, y);

	// Only notify observers if the instance actually moved. We noted that many
	// times the cell end up in the exactly same position.
	if (!dontNotifyObservers) {
		if (preivousX != physicalCell.getCoordinate(LOWER, X) || preivousY != physicalCell.getCoordinate(LOWER, Y)) {
			// Notify observers...
			for (auto &f : data->callbackPostInstanceMoved)
				std::get<1>(f) (physicalCell);
		} // end if
	} // end if	
} // end method

// -----------------------------------------------------------------------------

inline void PhysicalDesign::placeCell(Rsyn::Cell cell, const DBU x, const DBU y, const bool dontNotifyObservers) {
	placeCell(getPhysicalCell(cell), x, y, dontNotifyObservers);
} // end method	

// -----------------------------------------------------------------------------

inline void PhysicalDesign::placeCell(Rsyn::PhysicalCell physicalCell, const DBUxy pos, const bool dontNotifyObservers) {
	placeCell(physicalCell, pos[X], pos[Y], dontNotifyObservers);
} // end method

// -----------------------------------------------------------------------------

inline void PhysicalDesign::placeCell(Rsyn::Cell cell, const DBUxy pos, const bool dontNotifyObservers) {
	placeCell(getPhysicalCell(cell), pos[X], pos[Y], dontNotifyObservers);
} // end method

// -----------------------------------------------------------------------------

inline void PhysicalDesign::notifyObservers(Rsyn::PhysicalInstance instance) {
	// Notify observers...
	for (auto &f : data->callbackPostInstanceMoved)
		std::get<1>(f) (instance);
} // end method	

// -----------------------------------------------------------------------------

inline void PhysicalDesign::notifyObservers(Rsyn::PhysicalInstance instance, const PostInstanceMovedCallbackHandler &ignoreObserver) {
	// Notify observers...
	for (PostInstanceMovedCallbackHandler it = data->callbackPostInstanceMoved.begin();
		it != data->callbackPostInstanceMoved.end(); it++) {
		if (it != ignoreObserver) {
			std::get<1>(*it)(instance);
		} // end if
	} // end for
} // end method
// -----------------------------------------------------------------------------

inline PhysicalDesign::PostInstanceMovedCallbackHandler
PhysicalDesign::addPostInstanceMovedCallback(const int priority, PostInstanceMovedCallback f) {

	// We want to compare only the first element of the tuple. The default
	// comparator tries to compare all elements.
	auto comparator = [](
		const std::tuple<int, PhysicalDesign::PostInstanceMovedCallback> &left,
		const std::tuple<int, PhysicalDesign::PostInstanceMovedCallback> &right
		) -> bool {
			return std::get<0>(left) < std::get<0>(right);
		};

	data->callbackPostInstanceMoved.push_back(std::make_tuple(priority, f));
	PhysicalDesign::PostInstanceMovedCallbackHandler handler = std::prev(
		data->callbackPostInstanceMoved.end());
	data->callbackPostInstanceMoved.sort(comparator);
	return handler;
} // end method

} // end namespace 
