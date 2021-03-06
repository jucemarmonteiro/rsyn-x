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
 
#ifndef ICCAD15_EARLY_OPTO_H
#define ICCAD15_EARLY_OPTO_H

#include "rsyn/engine/Engine.h"
#include "rsyn/phy/PhysicalService.h"

namespace Rsyn {
class LibraryCharacterizer;
class Timer;
class RoutingEstimator;
} // end namespace

namespace ICCAD15 {

class Infrastructure;

class EarlyOpto : public Rsyn::Process {
private:
	Rsyn::Engine engine;
	Infrastructure *infra;
	Rsyn::Design design;
	Rsyn::Module module;
	Rsyn::PhysicalDesign phDesign;
	Rsyn::Timer *timer;
	Rsyn::RoutingEstimator *routingEstimator;
	Rsyn::PhysicalService *physical;
	const Rsyn::LibraryCharacterizer * libc;

	void stepSkewOptimization();
	void stepIterativeSpreading();
	void stepRegisterSwap();
	void stepRegisterToRegisterPathFix();	
	
	void runEarlySkewOptimization();
	void runEarlyWireOptimization();
	void runEarlyLocalSkewOptimizationByFlipFlopSwapping();
	void runEarlySpreadingIterative(const bool dontMoveLcbs);
	int runEarlySpreading(const bool dontMoveLcbs);
	
	void doLocalSkewOptimization(Rsyn::Cell lcb);
	double runEarlySpreading_ComputeCost(Rsyn::Cell cell);
	
public:
	
	virtual bool run(Rsyn::Engine engine, const Rsyn::Json &params);
	
}; // end class

} // end namespace

#endif