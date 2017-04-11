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
 
#include "rsyn/gui/frame/MainFrame.h"

// HOW-TO
// To register a canvas overlay, just include its .h below and add a call
// to "registerOverlay<T>(name, visibility)" where T is the canvas overlay class 
// name, name is the overlay label and visibility is the initial visibility 
// state of the overlay.

#include "rsyn/gui/canvas/overlay/Instance.h"
#include "rsyn/gui/canvas/overlay/Region.h"
#include "rsyn/gui/canvas/overlay/DensityGridOverlay.h"

#include "x/gui/overlay/Abu.h"
#include "x/gui/overlay/Jezz.h"
#include "x/gui/overlay/OverlapRemoverOverlay.h"
#include "x/gui/overlay/RoutingEstimatorOverlay.h"

// Overlays
void MainFrame::registerAllOverlays() {
	// Default overlays.
	registerOverlay<InstanceOverlay>("Instances", true);
	registerOverlay<RegionOverlay>("Region", true);
	registerOverlay<AbuOverlay>("Abu");
	registerOverlay<DensityGridOverlay>("Density Grid");
	
	// Non-default overlays.
	registerOverlay<JezzOverlay>("Jezz");
	registerOverlay<OverlapRemoverOverlay>("Macro Overlap");
	registerOverlay<RoutingEstimatorOverlay>("Routing estimator overlay");
} // end method