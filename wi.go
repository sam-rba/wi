package main

import "github.com/sam-rba/unit"

const (
	// Specific heat of dry mixture at constant pressure at T=273K.
	c_p_dfm = c_p_air

	// Specific heat of dry air at constant pressure at T=273K.
	c_p_air unit.SpecificHeat = 1006 * unit.JoulesPerKiloGramKelvin

	// Specific heat of water vapor at constant pressure at 273K.
	c_p_vap unit.SpecificHeat = 1805 * unit.JoulesPerKiloGramKelvin
)

const (
	// Enthalpy of vaporisation of water at T=273K.
	l_w unit.Enthalpy = 2501 * unit.KiloJoulesPerKiloGramKelvin
)

const (
	// Molar mass ratio.
	ɑ_w = M_w / M_air

	// Molar mass of water vapor at T=273K.
	M_w unit.MolarMass = 18015300 * unit.MicroGramPerMol

	// Molar mass of dry air at T=273K.
	M_air unit.MolarMass = 28964500 * unit.MicroGramPerMol
)
