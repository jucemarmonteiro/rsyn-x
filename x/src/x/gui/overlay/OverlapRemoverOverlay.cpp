/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   OverlapRemoverOverlay.cpp
 * Author: mpfogaca
 * 
 * Created on 17 de Outubro de 2016, 14:55
 */

#include "OverlapRemoverOverlay.h"
#include "rsyn/phy/PhysicalService.h"
OverlapRemoverOverlay::OverlapRemoverOverlay() {
} // ens constructor

//------------------------------------------------------------------------------

bool OverlapRemoverOverlay::init(PhysicalCanvasGL* canvas, nlohmann::json& properties) {
	Rsyn::Engine clsEngine = canvas->getEngine();

	// Jucemar - 03/26/17 -> Initializes overlay only if physical design is running. 
	if(!clsEngine.isServiceRunning("rsyn.physical")) 
		return false;
	
	ICCAD15::Infrastructure* clsInfrastructure = clsEngine.getService("ufrgs.ispd16.infra", Rsyn::SERVICE_OPTIONAL);
	if(!clsInfrastructure)
		return false;
	
	Rsyn::PhysicalService* physical = clsEngine.getService("rsyn.physical", Rsyn::SERVICE_OPTIONAL);
	if(!physical)
		return false;
	
	clsBlockageControl = clsEngine.getService("ufrgs.blockageControl", Rsyn::SERVICE_OPTIONAL);
	if(!clsBlockageControl)
		return false;
	
	clsPhysicalDesign = physical->getPhysicalDesign();	
	clsModule = clsEngine.getDesign().getTopModule();
	
	return true;
} // end method

//------------------------------------------------------------------------------

void OverlapRemoverOverlay::config(const nlohmann::json& params) {
} // end method

//------------------------------------------------------------------------------

void OverlapRemoverOverlay::render(PhysicalCanvasGL* canvas) {
	if (!canvas->getEngine())
		return;
	
	renderOverlaps();
} // end method

//------------------------------------------------------------------------------

void OverlapRemoverOverlay::renderOverlaps() {
	
	for (Rsyn::Instance instance : clsModule.allInstances()) {
		if (instance.getType() != Rsyn::InstanceType::CELL ) 
			continue;
			
		if(clsBlockageControl->hasOverlapWithMacro(instance.asCell())) {
			Rsyn::PhysicalCell pCell = clsPhysicalDesign.getPhysicalCell(instance.asCell());
			markOverlap(
				pCell.getCenter(X),
				pCell.getCenter(Y),
				pCell.getLength(X) + 5);
		}
	} // end for
} // end method



