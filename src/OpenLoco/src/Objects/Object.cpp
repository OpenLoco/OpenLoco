#include "Object.h"
#include <string_view>
#include <unordered_map>

namespace OpenLoco
{
    static const std::unordered_map<std::string_view, const uint8_t> _vanillaObjects = {
        // Type 0 (interfaceSkin)
        { "INTERDEF", 0 }, // "Default Interface Style"

        // Type 1 (sound)
        { "SNDA1   ", 1 }, // "Aircraft sound 1"
        { "SNDA2   ", 1 }, // "Aircraft sound 2"
        { "SNDA3   ", 1 }, // "Aircraft sound 3"
        { "SNDTD1  ", 1 }, // "Aircraft touchdown sound 1"
        { "SNDTD2  ", 1 }, // "Aircraft touchdown sound 2"
        { "SNDH4   ", 1 }, // "Boat Horn sound 4"
        { "SNDH5   ", 1 }, // "Boat Horn sound 5"
        { "SNDH6   ", 1 }, // "Boat Horn sound 6"
        { "SNDCH1A ", 1 }, // "Chuff 1 (a)"
        { "SNDCH1AS", 1 }, // "Chuff 1 (a-st)"
        { "SNDCH1B ", 1 }, // "Chuff 1 (b)"
        { "SNDCH1BS", 1 }, // "Chuff 1 (b-st)"
        { "SNDCH1C ", 1 }, // "Chuff 1 (c)"
        { "SNDCH1CS", 1 }, // "Chuff 1 (c-st)"
        { "SNDCH1D ", 1 }, // "Chuff 1 (d)"
        { "SNDCH1DS", 1 }, // "Chuff 1 (d-st)"
        { "SNDD1   ", 1 }, // "Diesel sound 1"
        { "SNDD10  ", 1 }, // "Diesel sound 10"
        { "SNDD11  ", 1 }, // "Diesel sound 11"
        { "SNDD12  ", 1 }, // "Diesel sound 12"
        { "SNDD2   ", 1 }, // "Diesel sound 2"
        { "SNDD3   ", 1 }, // "Diesel sound 3"
        { "SNDD4   ", 1 }, // "Diesel sound 4"
        { "SNDD5   ", 1 }, // "Diesel sound 5"
        { "SNDD6   ", 1 }, // "Diesel sound 6"
        { "SNDD7   ", 1 }, // "Diesel sound 7"
        { "SNDD8   ", 1 }, // "Diesel sound 8"
        { "SNDD9   ", 1 }, // "Diesel sound 9"
        { "SNDE1   ", 1 }, // "Electric sound 1"
        { "SNDH1   ", 1 }, // "Loco horn sound 1"
        { "SNDH2   ", 1 }, // "Loco horn sound 2"
        { "SNDH3   ", 1 }, // "Loco horn sound 3"
        { "SNDW1   ", 1 }, // "Loco whistle sound 1"
        { "SNDW2   ", 1 }, // "Loco whistle sound 2"
        { "SNDW3   ", 1 }, // "Loco whistle sound 3"
        { "SNDW4   ", 1 }, // "Loco whistle sound 4"
        { "SNDW5   ", 1 }, // "Loco whistle sound 5"
        { "SNDW6   ", 1 }, // "Loco whistle sound 6"
        { "SNDW7   ", 1 }, // "Loco whistle sound 7"
        { "SNDW8   ", 1 }, // "Loco whistle sound 8"
        { "SNDS1   ", 1 }, // "Steam idle sound 1"
        { "SNDTR1  ", 1 }, // "Track sound 1"

        // Type 2 (currency)
        { "CURRDMRK", 2 }, // "Deutschmark"
        { "CURRDOLL", 2 }, // "Dollars"
        { "CURREURO", 2 }, // "Euros"
        { "CURRFREN", 2 }, // "Franc"
        { "CURRGUIL", 2 }, // "Guilders"
        { "CURRKRON", 2 }, // "Krona"
        { "CURRLIRA", 2 }, // "Lira"
        { "CURRPSTA", 2 }, // "Peseta"
        { "CURRPNDS", 2 }, // "Pounds"
        { "CURRNTDL", 2 }, // "Taiwanese Dollars"
        { "CURRWON ", 2 }, // "Won"
        { "CURRYEN ", 2 }, // "Yen"
        { "CURRZLOT", 2 }, // "Zlote"

        // Type 3 (steam)
        { "EXH1    ", 3 }, // "Diesel Exhaust"
        { "EXH2    ", 3 }, // "Diesel Exhaust"
        { "SPARK1  ", 3 }, // "Electric Spark"
        { "WWAKE1  ", 3 }, // "Ship Wake 1"
        { "STEAM   ", 3 }, // "Steam Puff"

        // Type 4 (cliffEdge)
        { "LSBROWN ", 4 }, // "Brown Rock"
        { "LSROCK  ", 4 }, // "Rock"

        // Type 5 (water)
        { "WATER1  ", 5 }, // "Water"

        // Type 6 (land)
        { "ROCK2   ", 6 }, // "Brown Rock"
        { "GRASSBR ", 6 }, // "Dry Grass"
        { "GRASS1  ", 6 }, // "Grass"
        { "ROCK1   ", 6 }, // "Grey Rock"
        { "SAND1   ", 6 }, // "Sand"

        // Type 7 (townNames)
        { "ETOWNNAM", 7 }, // "English style town names"
        { "FTOWNNAM", 7 }, // "French style town names"
        { "GTOWNNAM", 7 }, // "German style town names"
        { "LTOWNNAM", 7 }, // "Latin style town names"
        { "ATOWNNAM", 7 }, // "North-American style town names"
        { "STOWNNAM", 7 }, // "Silly style town names"

        // Type 8 (cargo)
        { "CHEMICAL", 8 }, // "Chemicals"
        { "COAL    ", 8 }, // "Coal"
        { "FOOD    ", 8 }, // "Food"
        { "GOODS   ", 8 }, // "Goods"
        { "GRAIN   ", 8 }, // "Grain"
        { "GRAPES  ", 8 }, // "Grapes"
        { "IRONORE ", 8 }, // "Iron Ore"
        { "LIVESTCK", 8 }, // "Livestock"
        { "TIMBER  ", 8 }, // "Lumber"
        { "MAIL    ", 8 }, // "Mail"
        { "OIL     ", 8 }, // "Oil"
        { "PAPER   ", 8 }, // "Paper"
        { "PASS    ", 8 }, // "Passengers"
        { "STEEL   ", 8 }, // "Steel"

        // Type 9 (wall)
        { "FENCE1  ", 9 }, // "Fence"
        { "FENCE1G ", 9 }, // "Fence"
        { "SECFENCE", 9 }, // "Security Fence"
        { "SECFENCG", 9 }, // "Security Fence"
        { "STONWALG", 9 }, // "Stone Wall"
        { "STONWALL", 9 }, // "Stone Wall"
        { "BRWNWALG", 9 }, // "Wall"
        { "BRWNWALL", 9 }, // "Wall"

        // Type 10 (trackSignal)
        { "SIGC3   ", 10 }, // "Colourlight"
        { "SIGCSW  ", 10 }, // "Colourlight"
        { "SIGCUS  ", 10 }, // "Colourlight"
        { "SIGC4   ", 10 }, // "Colourlight (4-aspect)"
        { "SIGSUS  ", 10 }, // "Semaphore"
        { "SIGSW   ", 10 }, // "Semaphore"
        { "SIGSL   ", 10 }, // "Semaphore (LQ)"
        { "SIGSDL  ", 10 }, // "Semaphore (LQD)"
        { "SIGS    ", 10 }, // "Semaphore (UQ)"
        { "SIGSD   ", 10 }, // "Semaphore (UQD)"

        // Type 11 (levelCrossing)
        { "LCROSS3 ", 11 }, // "Level Crossing Gates"
        { "LCROSS1 ", 11 }, // "Level Crossing Lights"
        { "LCROSS2 ", 11 }, // "Level Crossing Lights"
        { "LCROSS4 ", 11 }, // "Level Crossing Signs"

        // Type 12 (streetLight)
        { "SLIGHT1 ", 12 }, // "Street Lights"

        // Type 13 (tunnel)
        { "TUNNEL1 ", 13 }, // "Tunnel"
        { "TUNNEL2 ", 13 }, // "Tunnel"

        // Type 14 (bridge)
        { "BRDGBRCK", 14 }, // "Brick Bridge"
        { "BRDGSTAR", 14 }, // "Steel Arch Bridge"
        { "BRDGGIRD", 14 }, // "Steel Girder Bridge"
        { "BRDGSUSP", 14 }, // "Suspension Bridge"
        { "BRDGWOOD", 14 }, // "Wooden Bridge"

        // Type 15 (trackStation)
        { "TRSTAT1 ", 15 }, // "City Station"
        { "TRSTAT4 ", 15 }, // "City Station"
        { "TRSTAT5 ", 15 }, // "Station"
        { "TRSTAT2 ", 15 }, // "Station"
        { "TRSTAT3 ", 15 }, // "Station"

        // Type 16 (trackExtra)
        { "TREX3RL ", 16 }, // "Electric 3rd Rail"
        { "TREXCAT1", 16 }, // "Overhead Elec. Wires"
        { "TREXRACK", 16 }, // "Rack Rail"

        // Type 17 (track)
        { "TRACKNG ", 17 }, // "Narrow Gauge Track"
        { "TRACKST ", 17 }, // "Railway Track"

        // Type 18 (roadStation)
        { "RDSTATL1", 18 }, // "Cargo Loading Bay"
        { "RDSTATL2", 18 }, // "Cargo Loading Bay"
        { "RDSTATL3", 18 }, // "Cargo Loading Bay"
        { "BUSSTOP ", 18 }, // "Passenger Stop"
        { "RDSTAT1 ", 18 }, // "Passenger Terminus"
        { "RDSTAT2 ", 18 }, // "Passenger Terminus"
        { "RDSTAT3 ", 18 }, // "Passenger Terminus"

        // Type 19 (roadExtra)
        { "RDEXCAT1", 19 }, // "Overhead Elec. Wires"

        // Type 20 (road)
        { "ROADONE ", 20 }, // "One-Way Road"
        { "ROADUS1 ", 20 }, // "One-Way Road"
        { "ROADTMC ", 20 }, // "Road"
        { "ROADUS2 ", 20 }, // "Road"
        { "ROADRGH ", 20 }, // "Rough Road"
        { "ROADTRAM", 20 }, // "Tram Track"

        // Type 21 (airport)
        { "HPORT1  ", 21 }, // "City Heliport"
        { "AIRPORT4", 21 }, // "International Airport"
        { "AIRPORT1", 21 }, // "Large Airport"
        { "HPORT2  ", 21 }, // "Large Heliport"
        { "AIRPORT3", 21 }, // "Medium Airport"
        { "AIRPORT2", 21 }, // "Small Airport"

        // Type 22 (dock)
        { "SHIPST1 ", 22 }, // "Docks"

        // Type 23 (vehicle)
        { "2EPB    ", 23 }, // "2-EPB Electric Multiple Unit"
        { "JINTY   ", 23 }, // "3F 'Jinty'"
        { "707     ", 23 }, // "707"
        { "737     ", 23 }, // "737"
        { "747     ", 23 }, // "747"
        { "777     ", 23 }, // "777"
        { "A320    ", 23 }, // "A320"
        { "A380    ", 23 }, // "A380"
        { "A5      ", 23 }, // "A5 Tri-Motor"
        { "AB139   ", 23 }, // "AB139 Helicopter"
        { "APT1    ", 23 }, // "APT Driving Carriage"
        { "APT2    ", 23 }, // "APT Passenger Carriage"
        { "APT3    ", 23 }, // "APT Power Car"
        { "AE47    ", 23 }, // "Ae 4/7"
        { "AILSA1  ", 23 }, // "Ailsa Bus"
        { "BA146   ", 23 }, // "BAe 146"
        { "4MT     ", 23 }, // "BR Standard Class 4MT"
        { "BALDWIN1", 23 }, // "Baldwin 2-8-0"
        { "TRAM1   ", 23 }, // "Be 4/4 Tram"
        { "TRAM4   ", 23 }, // "Be 4/6 Tram"
        { "GOODS2  ", 23 }, // "Boxcar"
        { "GOODS3  ", 23 }, // "Boxcar"
        { "COALSH  ", 23 }, // "Bulk Ship"
        { "C130    ", 23 }, // "C-130 Hercules"
        { "CARGOSH1", 23 }, // "Cargo Ship"
        { "CTRUCK1 ", 23 }, // "Cattle Truck"
        { "CTRUCK2 ", 23 }, // "Cattle Truck"
        { "CTRUCK3 ", 23 }, // "Cattle Truck"
        { "CTRUCK4 ", 23 }, // "Cattle Truck"
        { "TRAM2   ", 23 }, // "Ce 2/2 Tram"
        { "CE68    ", 23 }, // "Ce 6/8 Crocodile"
        { "ALCOCENT", 23 }, // "Century"
        { "114     ", 23 }, // "Class 114 Diesel Multiple Unit"
        { "142     ", 23 }, // "Class 142 Diesel Railbus"
        { "158     ", 23 }, // "Class 158 Diesel Multiple Unit"
        { "CL20    ", 23 }, // "Class 20"
        { "CL37    ", 23 }, // "Class 37"
        { "CL47    ", 23 }, // "Class 47"
        { "508     ", 23 }, // "Class 508 Electric Multiple Unit"
        { "CL55    ", 23 }, // "Class 55 'Deltic'"
        { "CL58    ", 23 }, // "Class 58"
        { "656     ", 23 }, // "Class 656"
        { "CL67    ", 23 }, // "Class 67"
        { "CL71    ", 23 }, // "Class 71"
        { "CL85    ", 23 }, // "Class 85"
        { "CL86    ", 23 }, // "Class 86"
        { "CL90    ", 23 }, // "Class 90"
        { "E8      ", 23 }, // "Class E8"
        { "CLIPPER ", 23 }, // "Clipper"
        { "TRAMCOMB", 23 }, // "Combi Tram"
        { "COMET   ", 23 }, // "Comet"
        { "CONCOR  ", 23 }, // "Concorde"
        { "TRAM3   ", 23 }, // "Coronation Tram"
        { "GRAINHP2", 23 }, // "Covered Hopper Wagon"
        { "GRAINHP1", 23 }, // "Covered Hopper Wagon"
        { "DC3     ", 23 }, // "DC-3"
        { "DH16    ", 23 }, // "DH-16"
        { "DASH7   ", 23 }, // "Dash-7"
        { "DEH46   ", 23 }, // "Deh 4/6"
        { "FERRY1  ", 23 }, // "Diesel Ferry"
        { "EWIVDT  ", 23 }, // "EW-IV Driving Carriage"
        { "EWIV    ", 23 }, // "EW-IV Passenger Carriage"
        { "EB35    ", 23 }, // "Eb 3/5"
        { "ESTAR2  ", 23 }, // "Electra-Star Passenger Carriage"
        { "ESTAR1  ", 23 }, // "Electra-Star Power Car"
        { "EMU1    ", 23 }, // "Electric Multiple Unit"
        { "F27     ", 23 }, // "F.27"
        { "F7      ", 23 }, // "F.7"
        { "FTRUCK1 ", 23 }, // "Flatbed Truck"
        { "FTRUCK2 ", 23 }, // "Flatbed Truck"
        { "FTRUCK3 ", 23 }, // "Flatbed Truck"
        { "FTRUCK4 ", 23 }, // "Flatbed Truck"
        { "FLATBED1", 23 }, // "Flatbed Wagon"
        { "FLATBED2", 23 }, // "Flatbed Wagon"
        { "FLTBEDN1", 23 }, // "Flatbed Wagon"
        { "4F      ", 23 }, // "Fowler 4F"
        { "CLASSIC ", 23 }, // "GM Classic"
        { "GE442   ", 23 }, // "Ge 4/4 II"
        { "GE443   ", 23 }, // "Ge 4/4 III"
        { "GE66    ", 23 }, // "Ge 6/6"
        { "GTRUCK1 ", 23 }, // "Goods Truck"
        { "GTRUCK2 ", 23 }, // "Goods Truck"
        { "GTRUCK3 ", 23 }, // "Goods Truck"
        { "GTRUCK4 ", 23 }, // "Goods Truck"
        { "PVAN2   ", 23 }, // "Goods Van"
        { "GOODS1  ", 23 }, // "Goods Wagon"
        { "GOODSN1 ", 23 }, // "Goods Wagon"
        { "A3      ", 23 }, // "Gresley A3"
        { "A4      ", 23 }, // "Gresley A4"
        { "V2      ", 23 }, // "Gresley V2"
        { "HGE44   ", 23 }, // "HGe 4/4"
        { "HGE442  ", 23 }, // "HGe 4/4 II"
        { "HST     ", 23 }, // "HST Power Car"
        { "HTRUCK1 ", 23 }, // "Hopper Truck"
        { "HTRUCK2 ", 23 }, // "Hopper Truck"
        { "HTRUCK3 ", 23 }, // "Hopper Truck"
        { "HTRUCK4 ", 23 }, // "Hopper Truck"
        { "HOPPER  ", 23 }, // "Hopper Wagon"
        { "HOPPER2 ", 23 }, // "Hopper Wagon"
        { "HFOIL1  ", 23 }, // "Hydrofoil"
        { "JU52    ", 23 }, // "JU52"
        { "JFOIL1  ", 23 }, // "Jetfoil"
        { "LEOP1   ", 23 }, // "Leopard Bus"
        { "CATTLEN1", 23 }, // "Livestock Wagon"
        { "CATTLE1 ", 23 }, // "Livestock Wagon"
        { "CATTLE3 ", 23 }, // "Livestock Wagon"
        { "MAILUS1 ", 23 }, // "Mail Car"
        { "MAILUS2 ", 23 }, // "Mail Car"
        { "MTRUCK1 ", 23 }, // "Mail Truck"
        { "MTRUCK2 ", 23 }, // "Mail Truck"
        { "MTRUCK3 ", 23 }, // "Mail Truck"
        { "CARGOSH2", 23 }, // "Merchant Freighter"
        { "MK1     ", 23 }, // "Mk1 Passenger Carriage"
        { "MK2     ", 23 }, // "Mk2 Passenger Carriage"
        { "MK3     ", 23 }, // "Mk3 Passenger Carriage"
        { "36R     ", 23 }, // "Model 36R Bus"
        { "OILSH   ", 23 }, // "Oil Tanker"
        { "USPACIF ", 23 }, // "Pacific"
        { "PVAN1   ", 23 }, // "Parcel Van"
        { "SWISS5  ", 23 }, // "Passenger Carriage"
        { "PCAR1   ", 23 }, // "Passenger Carriage"
        { "PCAR2   ", 23 }, // "Passenger Carriage"
        { "PCARSW1 ", 23 }, // "Passenger Carriage"
        { "PCARUS1 ", 23 }, //  "Passenger Carriage"
        { "PCARUS2 ", 23 }, // "Passenger Carriage"
        { "SWISS1  ", 23 }, // "Passenger Carriage"
        { "SWISS2  ", 23 }, // "Passenger Carriage"
        { "SWISS3  ", 23 }, // "Passenger Carriage"
        { "SWISS4  ", 23 }, // "Passenger Carriage"
        { "MAILSW1 ", 23 }, // "Postal Van"
        { "MAILSW4 ", 23 }, // "Postal Van"
        { "RBE24   ", 23 }, // "RBe 2/4 Railcar"
        { "RTMASTER", 23 }, // "RMT Bus"
        { "RE441   ", 23 }, // "Re 4/4 I"
        { "RE442   ", 23 }, // "Re 4/4 II"
        { "460     ", 23 }, // "Re 460"
        { "COPTER1 ", 23 }, // "S55 Helicopter"
        { "SD70MAC ", 23 }, // "SD70"
        { "C33     ", 23 }, // "SLM C 3/3"
        { "C56     ", 23 }, // "SLM C 5/6"
        { "HG23    ", 23 }, // "SLM HG 2/3"
        { "HCRAFT1 ", 23 }, // "SRN4 Hovercraft"
        { "SHINKT0 ", 23 }, // "Shinkansen Series 0 Powered Carriage"
        { "SHINKF0 ", 23 }, // "Shinkansen Series 0 Powered Driving Carriage"
        { "SPECIAL ", 23 }, // "Special 2-4-2"
        { "BLACK5  ", 23 }, // "Stanier Black 5"
        { "STANCORR", 23 }, // "Stanier Coronation Pacific"
        { "JUBILEE ", 23 }, // "Stanier Jubilee"
        { "FERRY2  ", 23 }, // "Steam Ferry"
        { "ST8FT   ", 23 }, // "Stirling 8ft"
        { "CATTLE2 ", 23 }, // "Stockcar"
        { "TDH5301 ", 23 }, // "TDH 5301 Bus"
        { "TGV2    ", 23 }, // "TGV Passenger Carriage"
        { "TGV1    ", 23 }, // "TGV Power Car"
        { "TTRAIL2 ", 23 }, // "Tanker Trailer"
        { "TTRUCK1 ", 23 }, // "Tanker Truck"
        { "TTRUCK2 ", 23 }, // "Tanker Truck"
        { "TTRUCK3 ", 23 }, // "Tanker Truck"
        { "TTRUCK4 ", 23 }, // "Tanker Truck"
        { "OIL1    ", 23 }, // "Tanker Wagon"
        { "OIL2    ", 23 }, // "Tanker Wagon"
        { "OILN1   ", 23 }, // "Tanker Wagon"
        { "VISCOUNT", 23 }, // "Viscount"
        { "VULCAN  ", 23 }, // "Vulcan VSD Bus"
        { "WMCBUS  ", 23 }, // "WMC Bus"
        { "COAL1   ", 23 }, // "Wagon"
        { "OPENN1  ", 23 }, // "Wagon"

        // Type 24 (tree)
        { "BSPRUCE ", 24 }, // "Blue Spruce Tree"
        { "CACTUS1 ", 24 }, // "Cactus"
        { "CACTUS2 ", 24 }, // "Cactus"
        { "CACTUS3 ", 24 }, // "Cactus"
        { "LCEDAR  ", 24 }, // "Cedar of Lebanon Tree"
        { "CHESTNUT", 24 }, // "Chestnut Tree"
        { "BEECH   ", 24 }, // "Common Beech Tree"
        { "COPBEACH", 24 }, // "Copper Beach Tree"
        { "WILLOW  ", 24 }, // "Crack Willow Tree"
        { "DREDWOOD", 24 }, // "Dawn Redwood Tree"
        { "ELM     ", 24 }, // "Elm Tree"
        { "FRISIA  ", 24 }, // "Frisia Tree"
        { "HCYPRESS", 24 }, // "Golden Hinoki Cypress Tree"
        { "GFIR    ", 24 }, // "Grand Fir Tree"
        { "HSTRAW  ", 24 }, // "Hydrid Strawberry Tree"
        { "IYEW    ", 24 }, // "Irish Yew Tree"
        { "ICYPRESS", 24 }, // "Italian Cypress Tree"
        { "JAPCRAB ", 24 }, // "Japanese Crab Tree"
        { "JOSHUA  ", 24 }, // "Joshua Tree"
        { "KBFIR   ", 24 }, // "King Boris's Fir Tree"
        { "NMAPLE  ", 24 }, // "Maple Tree"
        { "NASH    ", 24 }, // "Narrow Leaf Ash Tree"
        { "OAK     ", 24 }, // "Oak Tree"
        { "PCYPRESS", 24 }, // "Pond Cypress Tree"
        { "QPALM   ", 24 }, // "Queen Palm Tree"
        { "RCEDAR  ", 24 }, // "Red Cedar Tree"
        { "RFIR    ", 24 }, // "Red Fir Tree"
        { "ROBINIA ", 24 }, // "Robinia Tree"
        { "SCPINE  ", 24 }, // "Scots Pine Tree"
        { "BIRCH   ", 24 }, // "Silver Birch Tree"
        { "BIRCH2  ", 24 }, // "Silver Birch Tree"
        { "SPALM   ", 24 }, // "Silver Palm Tree"
        { "SVPINE  ", 24 }, // "Silver Pine Tree"
        { "SNOWBELL", 24 }, // "Snowbell Tree"
        { "SMAPLE  ", 24 }, // "Sugar Maple Tree"
        { "SCYPRESS", 24 }, // "Swamp Cypress Tree"
        { "WASH    ", 24 }, // "White Ash Tree"
        { "YEW     ", 24 }, // "Yew Tree"
        { "YUKKA   ", 24 }, // "Yukka Plant"

        // Type 25 (snow)
        { "SNOW    ", 25 }, // "Snow"

        // Type 26 (climate)
        { "CLIM1   ", 26 }, // "Alpine Climate"
        { "CLIM4   ", 26 }, // "Australian Climate"
        { "CLIM2   ", 26 }, // "British Climate"
        { "CLIM5   ", 26 }, // "Low Alpine Climate"
        { "CLIM3   ", 26 }, // "North American Climate"
        { "CLIM6   ", 26 }, // "North American, No Snow"

        // Type 27 (hillShapes)
        { "HS1     ", 27 }, // "Hill Shapes 1"

        // Type 28 (building)
        { "BLDALP8 ", 28 }, // "Apartments"
        { "BLDALP1 ", 28 }, // "Building"
        { "BLDHOU14", 28 }, // "Bungalow"
        { "BLDHOU18", 28 }, // "Bungalow"
        { "BLDCASTL", 28 }, // "Castle Ruins"
        { "BLDCRCH1", 28 }, // "Church"
        { "BLDCRCH2", 28 }, // "Church"
        { "BLDCRCH3", 28 }, // "Church"
        { "HQ1     ", 28 }, // "Company Headquarters"
        { "BLDHALL1", 28 }, // "Concert Hall"
        { "BLDALP3 ", 28 }, // "Cottage"
        { "BLDHOU10", 28 }, // "Cottage"
        { "BLDHOU20", 28 }, // "Cottage"
        { "BLDHOUS4", 28 }, // "Cottage"
        { "BLDHOUS6", 28 }, // "Cottage"
        { "BLDHOUS7", 28 }, // "Cottage"
        { "BLDHOUS9", 28 }, // "Cottage"
        { "BLDALP2 ", 28 }, // "Cottage"
        { "BLDHOUS8", 28 }, // "Cottages"
        { "BLDCTY21", 28 }, // "Court House"
        { "BLDPYLON", 28 }, // "Electricity Pylon"
        { "BLDCTY17", 28 }, // "Flats"
        { "BLDCTY20", 28 }, // "Flats"
        { "BLDFOUNT", 28 }, // "Fountain"
        { "BLDCTY18", 28 }, // "Hotel"
        { "BLDALP11", 28 }, // "House"
        { "BLDALP4 ", 28 }, // "House"
        { "BLDALP5 ", 28 }, // "House"
        { "BLDALP6 ", 28 }, // "House"
        { "BLDHOU12", 28 }, // "House"
        { "BLDHOU13", 28 }, // "House"
        { "BLDHOU15", 28 }, // "House"
        { "BLDHOU19", 28 }, // "House"
        { "BLDHOU21", 28 }, // "House"
        { "BLDHOU22", 28 }, // "House"
        { "BLDHOU24", 28 }, // "House"
        { "BLDHOU25", 28 }, // "House"
        { "BLDHOUS3", 28 }, // "House"
        { "BLDHOU23", 28 }, // "House"
        { "BLDLIGHT", 28 }, // "Lighthouse"
        { "BLDCTY13", 28 }, // "Office Block"
        { "BLDCTY19", 28 }, // "Office Block"
        { "BLDCTY2 ", 28 }, // "Office Block"
        { "BLDCTY28", 28 }, // "Office Block"
        { "BLDCTY29", 28 }, // "Office Block"
        { "BLDCTY3 ", 28 }, // "Office Block"
        { "BLDCTY4 ", 28 }, // "Office Block"
        { "BLDCTY5 ", 28 }, // "Office Block"
        { "BLDCTY15", 28 }, // "Office Building"
        { "BLDCTY12", 28 }, // "Office Tower"
        { "BLDCTY1 ", 28 }, // "Offices"
        { "BLDCTY10", 28 }, // "Offices"
        { "BLDCTY11", 28 }, // "Offices"
        { "BLDCTY14", 28 }, // "Offices"
        { "BLDCTY7 ", 28 }, // "Offices"
        { "BLDHOU11", 28 }, // "Semi-Detached Houses"
        { "BLDALP10", 28 }, // "Shops"
        { "BLDSHOP1", 28 }, // "Shops"
        { "BLDCTY25", 28 }, // "Shops and Apartments"
        { "BLDCTY26", 28 }, // "Shops and Apartments"
        { "BLDCTY27", 28 }, // "Shops and Apartments"
        { "BLDCTY6 ", 28 }, // "Shops and Offices"
        { "BLDCTY8 ", 28 }, // "Shops and Offices"
        { "BLDCTY9 ", 28 }, // "Shops and Offices"
        { "BLDHOU16", 28 }, // "Shops and Offices"
        { "BLDCTY16", 28 }, // "Shops and Offices"
        { "BLDCTY22", 28 }, // "Tenement Building"
        { "BLDCTY23", 28 }, // "Tenement Building"
        { "BLDCTY24", 28 }, // "Tenement Building"
        { "BLDHOU17", 28 }, // "Terraced Houses"
        { "BLDHOUS2", 28 }, // "Terraced Houses"
        { "BLDOFF1 ", 28 }, // "Tower Block Flats"
        { "BLDPARK1", 28 }, // "Town Square"
        { "BLDALP7 ", 28 }, // "Townhouse"
        { "BLDALP9 ", 28 }, // "Townhouse"
        { "BLDTRANS", 28 }, // "Transmitter"

        // Type 29 (scaffolding)
        { "SCAFDEF ", 29 }, // "Scaffolding"

        // Type 30 (industry)
        { "BREWERY ", 30 }, // "Brewery"
        { "CHEMWORK", 30 }, // "Chemical Works"
        { "COALMINE", 30 }, // "Coal Mines"
        { "COALPS  ", 30 }, // "Coal-Fired Power Station"
        { "FACTORY ", 30 }, // "Factory"
        { "WINDMILL", 30 }, // "Flour Mill"
        { "FOODPROC", 30 }, // "Food Processing Plant"
        { "OREMINE ", 30 }, // "Iron Ore Mines"
        { "STOCKFRM", 30 }, // "Livestock Farm"
        { "FOREST  ", 30 }, // "Managed Forest"
        { "OILREFIN", 30 }, // "Oil Refinery"
        { "OILRIG  ", 30 }, // "Oil Rig"
        { "OILWELL ", 30 }, // "Oil Wells"
        { "PAPERMIL", 30 }, // "Paper Mill"
        { "PRINTWRK", 30 }, // "Printing Works"
        { "SAWMILL ", 30 }, // "Sawmill"
        { "SKICENT ", 30 }, // "Ski Centre"
        { "STEELMIL", 30 }, // "Steel Mill"
        { "VINEYARD", 30 }, // "Vineyard"
        { "FARM    ", 30 }, // "Wheat Farm"
        { "FARMUS  ", 30 }, // "Wheat Farm"
        { "WINERY  ", 30 }, // "Winery"

        // Type 31 (region)
        { "REGALP  ", 31 }, // "Alpine Mountains"
        { "REGUK   ", 31 }, // "Great Britain"
        { "REGUS   ", 31 }, // "North America"

        // Type 32 (competitor)
        { "COMP9   ", 32 }, // "Agnes Armchair"
        { "COMP5   ", 32 }, // "Alfred Ashdown"
        { "COMP22  ", 32 }, // "Basil Boombox"
        { "COMP28  ", 32 }, // "Bo Buzzball"
        { "COMP23  ", 32 }, // "Cecil Cityscape"
        { "COMP10  ", 32 }, // "Charles Chilblain"
        { "COMP6   ", 32 }, // "Corina Cross"
        { "COMP27  ", 32 }, // "Daisy Dastardly"
        { "COMP7   ", 32 }, // "Dave Doorknob"
        { "COMP20  ", 32 }, // "Dorothy Dumblebum"
        { "COMP21  ", 32 }, // "Evelyn Eggburt"
        { "COMP12  ", 32 }, // "Foo Fandango"
        { "COMP33  ", 32 }, // "Freddy Fiddlestick"
        { "COMP24  ", 32 }, // "Gary Gangle"
        { "COMP4   ", 32 }, // "Guiseppe Gluck"
        { "COMP13  ", 32 }, // "Harriet Hatstand"
        { "COMP18  ", 32 }, // "Isaac Imperial"
        { "COMP8   ", 32 }, // "Jeremy Jewelfish"
        { "COMP2   ", 32 }, // "Jillie Jamjar"
        { "COMP14  ", 32 }, // "Kathy Kilowatt"
        { "COMP15  ", 32 }, // "Lizzy Leopard"
        { "COMP30  ", 32 }, // "Maggie Munchkins"
        { "COMP16  ", 32 }, // "Marmaduke Muddleshop"
        { "COMP26  ", 32 }, // "Mick Motormouth"
        { "COMP29  ", 32 }, // "Mr Big"
        { "COMP34  ", 32 }, // "Naomi Nevermore"
        { "COMP35  ", 32 }, // "Oliver Organic"
        { "COMP32  ", 32 }, // "Pete Pennyweight"
        { "COMP17  ", 32 }, // "Reginald Rottenfish"
        { "COMP38  ", 32 }, // "Ricky Ripoff"
        { "COMP1   ", 32 }, // "Ronnie Riddlesworth"
        { "COMP3   ", 32 }, // "Rosie Redbone"
        { "COMP36  ", 32 }, // "Sue Shaker"
        { "COMP31  ", 32 }, // "Theodore Thin"
        { "COMP25  ", 32 }, // "Theresa Tops"
        { "COMP37  ", 32 }, // "Tony Terrific"
        { "COMP11  ", 32 }, // "Walter Wellington"
        { "COMP19  ", 32 }, // "Wodger Whamjet"

        // Type 33 (scenarioText)
        { "STEX043 ", 33 }, // "Aerophobia"
        { "STEX024 ", 33 }, // "Bottleneck Blues"
        { "STEX038 ", 33 }, // "Boulder Breakers"
        { "STEX040 ", 33 }, // "Clifftop Climb"
        { "STEX021 ", 33 }, // "Cyclade Capers"
        { "STEX025 ", 33 }, // "Desert Delirium"
        { "STEX022 ", 33 }, // "Dodecan Diaries"
        { "STEX036 ", 33 }, // "Feed Flintrock"
        { "STEX003 ", 33 }, // "Great Britain & Ireland - 100 Year Challenge"
        { "STEX000 ", 33 }, // "Great Britain & Ireland 1900"
        { "STEX001 ", 33 }, // "Great Britain & Ireland 1930"
        { "STEX002 ", 33 }, // "Great Britain & Ireland 1955"
        { "STEX034 ", 33 }, // "Keystone Keys"
        { "STEX030 ", 33 }, // "Lost Worlds"
        { "STEX027 ", 33 }, // "Mixing Muscle"
        { "STEX023 ", 33 }, // "Mountain Mayhem"
        { "STEX007 ", 33 }, // "North America (East) - 100 Year Challenge"
        { "STEX004 ", 33 }, // "North America (East) 1900"
        { "STEX005 ", 33 }, // "North America (East) 1950"
        { "STEX006 ", 33 }, // "North America (East) 1965"
        { "STEX011 ", 33 }, // "North America (Midwest) - 100 Year Challenge"
        { "STEX008 ", 33 }, // "North America (Midwest) 1920"
        { "STEX009 ", 33 }, // "North America (Midwest) 1945"
        { "STEX010 ", 33 }, // "North America (Midwest) 1970"
        { "STEX015 ", 33 }, // "North America (West) - 100 Year Challenge"
        { "STEX012 ", 33 }, // "North America (West) 1910"
        { "STEX013 ", 33 }, // "North America (West) 1955"
        { "STEX014 ", 33 }, // "North America (West) 1980"
        { "STEX031 ", 33 }, // "Oil Oasis"
        { "STEX041 ", 33 }, // "Pothole Peril"
        { "STEX020 ", 33 }, // "Race to Read"
        { "STEX042 ", 33 }, // "Rails Against Roads"
        { "STEX037 ", 33 }, // "Sandbox Settler"
        { "STEX028 ", 33 }, // "Santarinos"
        { "STEX032 ", 33 }, // "Smiley Isley"
        { "STEX039 ", 33 }, // "Snowy Heights"
        { "STEX033 ", 33 }, // "Stepping Stones"
        { "STEX019 ", 33 }, // "Swiss Alps - 100 Year Challenge"
        { "STEX016 ", 33 }, // "Swiss Alps 1905"
        { "STEX017 ", 33 }, // "Swiss Alps 1930"
        { "STEX018 ", 33 }, // "Swiss Alps 1960"
        { "STEX026 ", 33 }, // "Vache and Vineyards"
        { "STEX035 ", 33 }, // "Vapid Volcano"
        { "STEX044 ", 33 }, // "Weatherworld"
        { "STEX029 ", 33 }, // "Yew Island"
    };

    bool ObjectHeader::isVanilla() const
    {
        auto search = _vanillaObjects.find(getName());
        return isVanilla = search != _vanillaObjects.end();
    }
}
