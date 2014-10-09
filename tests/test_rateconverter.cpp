/*
  Copyright 2014 SINTEF ICT, Applied Mathematics.
  Copyright 2014 Statoil ASA.

  This file is part of the Open Porous Media Project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE VoidageRateConversionTest

#include <opm/autodiff/RateConverter.hpp>

#include <opm/autodiff/BlackoilPropsAd.hpp>

#include <boost/test/unit_test.hpp>

#include <opm/core/grid/GridManager.hpp>
#include <opm/core/props/BlackoilPropertiesFromDeck.hpp>
#include <opm/core/utility/Units.hpp>
#include <opm/core/utility/parameters/ParameterGroup.hpp>
#include <opm/core/simulator/BlackoilState.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>


struct SetupSimple {
    SetupSimple()
    {
        Opm::ParserPtr parser(new Opm::Parser());
        deck = parser->parseFile("fluid.data");
        eclState.reset(new Opm::EclipseState(deck));

        param.disableOutput();
        param.insertParameter("init_rock"       , "false" );
        param.insertParameter("threephase_model", "simple");
        param.insertParameter("pvt_tab_size"    , "0"     );
        param.insertParameter("sat_tab_size"    , "0"     );
    }

    Opm::parameter::ParameterGroup  param;
    Opm::DeckConstPtr               deck;
    Opm::EclipseStateConstPtr       eclState;
};


template <class Setup>
struct TestFixture : public Setup
{
    TestFixture()
        : Setup()
        , grid (deck)
        , props(deck, eclState, *grid.c_grid(), param,
                param.getDefault("init_rock", false))
    {
    }

    using Setup::param;
    using Setup::deck;
    using Setup::eclState;

    Opm::GridManager                grid;
    Opm::BlackoilPropertiesFromDeck props;
};


BOOST_FIXTURE_TEST_CASE(Construction, TestFixture<SetupSimple>)
{
    typedef std::vector<int>                     Region;
    typedef Opm::BlackoilPropsAd                 Props;
    typedef Opm::RateConverter::
        SurfaceToReservoirVoidage<Props, Region> RCvrt;

    Region reg{ 0 };
    Props  ad_props(props);
    RCvrt  cvrt(ad_props, reg);
}


BOOST_FIXTURE_TEST_CASE(TwoPhaseII, TestFixture<SetupSimple>)
{
    // Immiscible and incompressible two-phase fluid
    typedef std::vector<int>                     Region;
    typedef Opm::BlackoilPropsAd                 Props;
    typedef Opm::RateConverter::
        SurfaceToReservoirVoidage<Props, Region> RCvrt;

    Region reg{ 0 };
    Props  ad_props(props);
    RCvrt  cvrt(ad_props, reg);

    Opm::BlackoilState x;
    x.init(*grid.c_grid(), 2);

    cvrt.defineState(x);

    std::vector<double> qs{1.0e3, 1.0e1};
    std::vector<double> coeff(qs.size(), 0.0);

    // Immiscible and incompressible: All coefficients are one (1),
    // irrespective of actual surface rates.
    cvrt.calcCoeff(qs, 0, coeff);
    BOOST_CHECK_CLOSE(coeff[0], 1.0, 1.0e-6);
    BOOST_CHECK_CLOSE(coeff[1], 1.0, 1.0e-6);
}