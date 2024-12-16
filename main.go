package main

import (
	"flag"
	"fmt"
	"gopkg.in/yaml.v2"
	"github.com/sam-rba/unit"
	"io"
	"os"
)

var configFile = flag.String("config", "wi.yaml", "YAML file containing engine parameters")

type Config struct {
	Displacement           string `yaml:"engine displacement"`
	WaterPressure          string `yaml:"water pressure"`
	MaxWaterVolumeFlowRate string `yaml:"max water volume flow rate"`
}

func main() {
	flag.Parse()

	config, err := readConfig(*configFile)
	if err != nil {
		fmt.Println("Error reading config:", err)
		return
	}
	fmt.Println(config)

	displacement, err := config.parseDisplacement()
	if err != nil {
		fmt.Println("Error parsing config:", err)
		return
	}

	P_water, err := config.parseWaterPressure()
	if err != nil {
		fmt.Println("Error parsing config:", err)
		return
	}
}

func readConfig(file string) (Config, error) {
	fmt.Println("Reading config from", file)

	f, err := os.Open(file)
	if err != nil {
		return Config{}, err
	}
	defer f.Close()

	data, err := io.ReadAll(f)
	if err != nil {
		return Config{}, err
	}

	var config Config
	err = yaml.UnmarshalStrict(data, &config)
	return config, err
}

func (c Config) parseDisplacement() (unit.Volume, error) {
	var v unit.Volume
	err := v.Set(c.Displacement)
	return v, err
}

func (c Config) parseWaterPressure() (unit.Pressure, error) {
	var p unit.Pressure
	err := p.Set(c.WaterPressure)
	return p, err
}
