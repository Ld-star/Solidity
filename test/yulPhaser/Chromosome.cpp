/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <test/yulPhaser/Common.h>

#include <tools/yulPhaser/Chromosome.h>
#include <tools/yulPhaser/SimulationRNG.h>

#include <libyul/optimiser/BlockFlattener.h>
#include <libyul/optimiser/ConditionalSimplifier.h>
#include <libyul/optimiser/ExpressionInliner.h>
#include <libyul/optimiser/ExpressionSimplifier.h>
#include <libyul/optimiser/ForLoopConditionOutOfBody.h>
#include <libyul/optimiser/ForLoopConditionOutOfBody.h>
#include <libyul/optimiser/ForLoopInitRewriter.h>
#include <libyul/optimiser/FunctionHoister.h>
#include <libyul/optimiser/LoopInvariantCodeMotion.h>
#include <libyul/optimiser/RedundantAssignEliminator.h>
#include <libyul/optimiser/Rematerialiser.h>
#include <libyul/optimiser/Suite.h>
#include <libyul/optimiser/StructuralSimplifier.h>
#include <libyul/optimiser/UnusedPruner.h>

#include <libsolutil/CommonIO.h>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace solidity::yul;
using namespace solidity::util;

namespace solidity::phaser::test
{

BOOST_AUTO_TEST_SUITE(Phaser)
BOOST_AUTO_TEST_SUITE(ChromosomeTest)

BOOST_AUTO_TEST_CASE(constructor_should_convert_from_string_to_optimisation_steps)
{
	vector<string> expectedSteps{
		ConditionalSimplifier::name,
		FunctionHoister::name,
		RedundantAssignEliminator::name,
		ForLoopConditionOutOfBody::name,
		Rematerialiser::name,
		ForLoopConditionOutOfBody::name,
		ExpressionSimplifier::name,
		ForLoopInitRewriter::name,
		LoopInvariantCodeMotion::name,
		ExpressionInliner::name
	};

	BOOST_TEST(Chromosome("ChrOmOsoMe").optimisationSteps() == expectedSteps);
}

BOOST_AUTO_TEST_CASE(makeRandom_should_return_different_chromosome_each_time)
{
	SimulationRNG::reset(1);
	for (size_t i = 0; i < 10; ++i)
		BOOST_TEST(Chromosome::makeRandom(100) != Chromosome::makeRandom(100));
}

BOOST_AUTO_TEST_CASE(makeRandom_should_use_every_possible_step_with_the_same_probability)
{
	SimulationRNG::reset(1);
	constexpr int samplesPerStep = 100;
	constexpr double relativeTolerance = 0.01;

	map<string, size_t> stepIndices = enumerateOptmisationSteps();
	auto chromosome = Chromosome::makeRandom(stepIndices.size() * samplesPerStep);

	vector<size_t> samples;
	for (auto& step: chromosome.optimisationSteps())
		samples.push_back(stepIndices.at(step));

	const double expectedValue = (stepIndices.size() - 1) / 2.0;
	const double variance = (stepIndices.size() * stepIndices.size() - 1) / 12.0;

	BOOST_TEST(abs(mean(samples) - expectedValue) < expectedValue * relativeTolerance);
	BOOST_TEST(abs(meanSquaredError(samples, expectedValue) - variance) < variance * relativeTolerance);
}

BOOST_AUTO_TEST_CASE(constructor_should_store_optimisation_steps)
{
	vector<string> steps = {
		StructuralSimplifier::name,
		BlockFlattener::name,
		UnusedPruner::name,
	};
	Chromosome chromosome(steps);

	BOOST_TEST(steps == chromosome.optimisationSteps());
}

BOOST_AUTO_TEST_CASE(constructor_should_allow_duplicate_steps)
{
	vector<string> steps = {
		StructuralSimplifier::name,
		StructuralSimplifier::name,
		BlockFlattener::name,
		UnusedPruner::name,
		BlockFlattener::name,
	};
	Chromosome chromosome(steps);

	BOOST_TEST(steps == chromosome.optimisationSteps());
}

BOOST_AUTO_TEST_CASE(output_operator_should_create_concise_and_unambiguous_string_representation)
{
	vector<string> allSteps;
	for (auto const& step: OptimiserSuite::allSteps())
		allSteps.push_back(step.first);
	Chromosome chromosome(allSteps);

	BOOST_TEST(chromosome.length() == allSteps.size());
	BOOST_TEST(chromosome.optimisationSteps() == allSteps);
	BOOST_TEST(toString(chromosome) == "flcCUnDvejsxIOoighTLMrmVatud");
}

BOOST_AUTO_TEST_CASE(randomOptimisationStep_should_return_each_step_with_same_probability)
{
	SimulationRNG::reset(1);
	constexpr int samplesPerStep = 100;
	constexpr double relativeTolerance = 0.01;

	map<string, size_t> stepIndices = enumerateOptmisationSteps();
	vector<size_t> samples;
	for (size_t i = 0; i <= stepIndices.size() * samplesPerStep; ++i)
		samples.push_back(stepIndices.at(Chromosome::randomOptimisationStep()));

	const double expectedValue = (stepIndices.size() - 1) / 2.0;
	const double variance = (stepIndices.size() * stepIndices.size() - 1) / 12.0;

	BOOST_TEST(abs(mean(samples) - expectedValue) < expectedValue * relativeTolerance);
	BOOST_TEST(abs(meanSquaredError(samples, expectedValue) - variance) < variance * relativeTolerance);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

}
