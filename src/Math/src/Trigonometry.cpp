#include "Trigonometry.hpp"
#include <array>

namespace OpenLoco::Math::Trigonometry
{
    namespace Data
    {
        // Excel Function =SIN((A1/16384)*2*PI())*32767
        // Where A1 is 0 : 4095
        // NOTE: this excel func is close but not identical
        // clang-format off
        // 0x00501B50
        static constexpr std::array<int16_t, kDirectionPrecisionHigh / 4> kQuarterSine = {
                0,    12,    25,    37,    50,    62,    75,    87,   100,   113,   125,   138,   150,   163,   175,   188,   201,   213,   226,   238,
              251,   263,   276,   289,   301,   314,   326,   339,   351,   364,   376,   389,   402,   414,   427,   439,   452,   464,   477,   490,
              502,   515,   527,   540,   552,   565,   578,   590,   603,   615,   628,   640,   653,   665,   678,   691,   703,   716,   728,   741,
              753,   766,   779,   791,   804,   816,   829,   841,   854,   866,   879,   892,   904,   917,   929,   942,   954,   967,   980,   992,
             1005,  1017,  1030,  1042,  1055,  1067,  1080,  1093,  1105,  1118,  1130,  1143,  1155,  1168,  1180,  1193,  1206,  1218,  1231,  1243,
             1256,  1268,  1281,  1293,  1306,  1319,  1331,  1344,  1356,  1369,  1381,  1394,  1406,  1419,  1432,  1444,  1457,  1469,  1482,  1494,
             1507,  1519,  1532,  1545,  1557,  1570,  1582,  1595,  1607,  1620,  1632,  1645,  1658,  1670,  1683,  1695,  1708,  1720,  1733,  1745,
             1758,  1770,  1783,  1796,  1808,  1821,  1833,  1846,  1858,  1871,  1883,  1896,  1908,  1921,  1934,  1946,  1959,  1971,  1984,  1996,
             2009,  2021,  2034,  2046,  2059,  2072,  2084,  2097,  2109,  2122,  2134,  2147,  2159,  2172,  2184,  2197,  2209,  2222,  2235,  2247,
             2260,  2272,  2285,  2297,  2310,  2322,  2335,  2347,  2360,  2372,  2385,  2397,  2410,  2423,  2435,  2448,  2460,  2473,  2485,  2498,
             2510,  2523,  2535,  2548,  2560,  2573,  2585,  2598,  2610,  2623,  2636,  2648,  2661,  2673,  2686,  2698,  2711,  2723,  2736,  2748,
             2761,  2773,  2786,  2798,  2811,  2823,  2836,  2848,  2861,  2873,  2886,  2898,  2911,  2924,  2936,  2949,  2961,  2974,  2986,  2999,
             3011,  3024,  3036,  3049,  3061,  3074,  3086,  3099,  3111,  3124,  3136,  3149,  3161,  3174,  3186,  3199,  3211,  3224,  3236,  3249,
             3261,  3274,  3286,  3299,  3311,  3324,  3336,  3349,  3361,  3374,  3386,  3399,  3411,  3424,  3436,  3449,  3461,  3474,  3486,  3499,
             3511,  3524,  3536,  3549,  3561,  3574,  3586,  3599,  3611,  3624,  3636,  3649,  3661,  3674,  3686,  3699,  3711,  3724,  3736,  3749,
             3761,  3774,  3786,  3798,  3811,  3823,  3836,  3848,  3861,  3873,  3886,  3898,  3911,  3923,  3936,  3948,  3961,  3973,  3986,  3998,
             4011,  4023,  4036,  4048,  4060,  4073,  4085,  4098,  4110,  4123,  4135,  4148,  4160,  4173,  4185,  4198,  4210,  4223,  4235,  4247,
             4260,  4272,  4285,  4297,  4310,  4322,  4335,  4347,  4360,  4372,  4384,  4397,  4409,  4422,  4434,  4447,  4459,  4472,  4484,  4497,
             4509,  4521,  4534,  4546,  4559,  4571,  4584,  4596,  4609,  4621,  4633,  4646,  4658,  4671,  4683,  4696,  4708,  4720,  4733,  4745,
             4758,  4770,  4783,  4795,  4807,  4820,  4832,  4845,  4857,  4870,  4882,  4894,  4907,  4919,  4932,  4944,  4957,  4969,  4981,  4994,
             5006,  5019,  5031,  5044,  5056,  5068,  5081,  5093,  5106,  5118,  5130,  5143,  5155,  5168,  5180,  5192,  5205,  5217,  5230,  5242,
             5255,  5267,  5279,  5292,  5304,  5317,  5329,  5341,  5354,  5366,  5379,  5391,  5403,  5416,  5428,  5440,  5453,  5465,  5478,  5490,
             5502,  5515,  5527,  5540,  5552,  5564,  5577,  5589,  5601,  5614,  5626,  5639,  5651,  5663,  5676,  5688,  5701,  5713,  5725,  5738,
             5750,  5762,  5775,  5787,  5799,  5812,  5824,  5837,  5849,  5861,  5874,  5886,  5898,  5911,  5923,  5935,  5948,  5960,  5973,  5985,
             5997,  6010,  6022,  6034,  6047,  6059,  6071,  6084,  6096,  6108,  6121,  6133,  6145,  6158,  6170,  6182,  6195,  6207,  6219,  6232,
             6244,  6256,  6269,  6281,  6293,  6306,  6318,  6330,  6343,  6355,  6367,  6380,  6392,  6404,  6417,  6429,  6441,  6454,  6466,  6478,
             6491,  6503,  6515,  6528,  6540,  6552,  6565,  6577,  6589,  6602,  6614,  6626,  6638,  6651,  6663,  6675,  6688,  6700,  6712,  6725,
             6737,  6749,  6761,  6774,  6786,  6798,  6811,  6823,  6835,  6847,  6860,  6872,  6884,  6897,  6909,  6921,  6933,  6946,  6958,  6970,
             6983,  6995,  7007,  7019,  7032,  7044,  7056,  7069,  7081,  7093,  7105,  7118,  7130,  7142,  7154,  7167,  7179,  7191,  7203,  7216,
             7228,  7240,  7252,  7265,  7277,  7289,  7301,  7314,  7326,  7338,  7350,  7363,  7375,  7387,  7399,  7412,  7424,  7436,  7448,  7461,
             7473,  7485,  7497,  7510,  7522,  7534,  7546,  7558,  7571,  7583,  7595,  7607,  7620,  7632,  7644,  7656,  7668,  7681,  7693,  7705,
             7717,  7730,  7742,  7754,  7766,  7778,  7791,  7803,  7815,  7827,  7839,  7852,  7864,  7876,  7888,  7900,  7913,  7925,  7937,  7949,
             7961,  7974,  7986,  7998,  8010,  8022,  8034,  8047,  8059,  8071,  8083,  8095,  8108,  8120,  8132,  8144,  8156,  8168,  8181,  8193,
             8205,  8217,  8229,  8241,  8254,  8266,  8278,  8290,  8302,  8314,  8327,  8339,  8351,  8363,  8375,  8387,  8399,  8412,  8424,  8436,
             8448,  8460,  8472,  8484,  8497,  8509,  8521,  8533,  8545,  8557,  8569,  8581,  8594,  8606,  8618,  8630,  8642,  8654,  8666,  8678,
             8691,  8703,  8715,  8727,  8739,  8751,  8763,  8775,  8787,  8800,  8812,  8824,  8836,  8848,  8860,  8872,  8884,  8896,  8908,  8921,
             8933,  8945,  8957,  8969,  8981,  8993,  9005,  9017,  9029,  9041,  9053,  9066,  9078,  9090,  9102,  9114,  9126,  9138,  9150,  9162,
             9174,  9186,  9198,  9210,  9222,  9234,  9247,  9259,  9271,  9283,  9295,  9307,  9319,  9331,  9343,  9355,  9367,  9379,  9391,  9403,
             9415,  9427,  9439,  9451,  9463,  9475,  9487,  9499,  9511,  9523,  9535,  9547,  9559,  9572,  9584,  9596,  9608,  9620,  9632,  9644,
             9656,  9668,  9680,  9692,  9704,  9716,  9728,  9740,  9752,  9764,  9776,  9788,  9800,  9812,  9824,  9836,  9848,  9860,  9872,  9883,
             9895,  9907,  9919,  9931,  9943,  9955,  9967,  9979,  9991, 10003, 10015, 10027, 10039, 10051, 10063, 10075, 10087, 10099, 10111, 10123,
            10135, 10147, 10159, 10171, 10183, 10195, 10206, 10218, 10230, 10242, 10254, 10266, 10278, 10290, 10302, 10314, 10326, 10338, 10350, 10362,
            10373, 10385, 10397, 10409, 10421, 10433, 10445, 10457, 10469, 10481, 10493, 10504, 10516, 10528, 10540, 10552, 10564, 10576, 10588, 10600,
            10612, 10623, 10635, 10647, 10659, 10671, 10683, 10695, 10707, 10718, 10730, 10742, 10754, 10766, 10778, 10790, 10802, 10813, 10825, 10837,
            10849, 10861, 10873, 10885, 10896, 10908, 10920, 10932, 10944, 10956, 10968, 10979, 10991, 11003, 11015, 11027, 11039, 11050, 11062, 11074,
            11086, 11098, 11109, 11121, 11133, 11145, 11157, 11169, 11180, 11192, 11204, 11216, 11228, 11239, 11251, 11263, 11275, 11287, 11298, 11310,
            11322, 11334, 11346, 11357, 11369, 11381, 11393, 11405, 11416, 11428, 11440, 11452, 11463, 11475, 11487, 11499, 11510, 11522, 11534, 11546,
            11558, 11569, 11581, 11593, 11605, 11616, 11628, 11640, 11652, 11663, 11675, 11687, 11699, 11710, 11722, 11734, 11745, 11757, 11769, 11781,
            11792, 11804, 11816, 11828, 11839, 11851, 11863, 11874, 11886, 11898, 11910, 11921, 11933, 11945, 11956, 11968, 11980, 11991, 12003, 12015,
            12026, 12038, 12050, 12062, 12073, 12085, 12097, 12108, 12120, 12132, 12143, 12155, 12167, 12178, 12190, 12202, 12213, 12225, 12237, 12248,
            12260, 12272, 12283, 12295, 12307, 12318, 12330, 12341, 12353, 12365, 12376, 12388, 12400, 12411, 12423, 12435, 12446, 12458, 12469, 12481,
            12493, 12504, 12516, 12527, 12539, 12551, 12562, 12574, 12586, 12597, 12609, 12620, 12632, 12643, 12655, 12667, 12678, 12690, 12701, 12713,
            12725, 12736, 12748, 12759, 12771, 12782, 12794, 12806, 12817, 12829, 12840, 12852, 12863, 12875, 12887, 12898, 12910, 12921, 12933, 12944,
            12956, 12967, 12979, 12990, 13002, 13014, 13025, 13037, 13048, 13060, 13071, 13083, 13094, 13106, 13117, 13129, 13140, 13152, 13163, 13175,
            13186, 13198, 13209, 13221, 13232, 13244, 13255, 13267, 13278, 13290, 13301, 13313, 13324, 13336, 13347, 13359, 13370, 13382, 13393, 13404,
            13416, 13427, 13439, 13450, 13462, 13473, 13485, 13496, 13508, 13519, 13531, 13542, 13553, 13565, 13576, 13588, 13599, 13611, 13622, 13633,
            13645, 13656, 13668, 13679, 13691, 13702, 13713, 13725, 13736, 13748, 13759, 13770, 13782, 13793, 13805, 13816, 13827, 13839, 13850, 13862,
            13873, 13884, 13896, 13907, 13918, 13930, 13941, 13953, 13964, 13975, 13987, 13998, 14009, 14021, 14032, 14043, 14055, 14066, 14078, 14089,
            14100, 14112, 14123, 14134, 14146, 14157, 14168, 14180, 14191, 14202, 14214, 14225, 14236, 14248, 14259, 14270, 14281, 14293, 14304, 14315,
            14327, 14338, 14349, 14361, 14372, 14383, 14394, 14406, 14417, 14428, 14440, 14451, 14462, 14473, 14485, 14496, 14507, 14518, 14530, 14541,
            14552, 14564, 14575, 14586, 14597, 14609, 14620, 14631, 14642, 14654, 14665, 14676, 14687, 14698, 14710, 14721, 14732, 14743, 14755, 14766,
            14777, 14788, 14799, 14811, 14822, 14833, 14844, 14855, 14867, 14878, 14889, 14900, 14911, 14923, 14934, 14945, 14956, 14967, 14979, 14990,
            15001, 15012, 15023, 15034, 15046, 15057, 15068, 15079, 15090, 15101, 15113, 15124, 15135, 15146, 15157, 15168, 15179, 15191, 15202, 15213,
            15224, 15235, 15246, 15257, 15268, 15280, 15291, 15302, 15313, 15324, 15335, 15346, 15357, 15368, 15379, 15391, 15402, 15413, 15424, 15435,
            15446, 15457, 15468, 15479, 15490, 15501, 15512, 15524, 15535, 15546, 15557, 15568, 15579, 15590, 15601, 15612, 15623, 15634, 15645, 15656,
            15667, 15678, 15689, 15700, 15711, 15722, 15733, 15744, 15755, 15766, 15777, 15788, 15799, 15810, 15821, 15832, 15843, 15854, 15865, 15876,
            15887, 15898, 15909, 15920, 15931, 15942, 15953, 15964, 15975, 15986, 15997, 16008, 16019, 16030, 16041, 16052, 16063, 16074, 16085, 16096,
            16107, 16118, 16129, 16140, 16151, 16161, 16172, 16183, 16194, 16205, 16216, 16227, 16238, 16249, 16260, 16271, 16282, 16292, 16303, 16314,
            16325, 16336, 16347, 16358, 16369, 16380, 16391, 16401, 16412, 16423, 16434, 16445, 16456, 16467, 16477, 16488, 16499, 16510, 16521, 16532,
            16543, 16553, 16564, 16575, 16586, 16597, 16608, 16618, 16629, 16640, 16651, 16662, 16673, 16683, 16694, 16705, 16716, 16727, 16737, 16748,
            16759, 16770, 16781, 16791, 16802, 16813, 16824, 16835, 16845, 16856, 16867, 16878, 16888, 16899, 16910, 16921, 16932, 16942, 16953, 16964,
            16975, 16985, 16996, 17007, 17017, 17028, 17039, 17050, 17060, 17071, 17082, 17093, 17103, 17114, 17125, 17135, 17146, 17157, 17168, 17178,
            17189, 17200, 17210, 17221, 17232, 17242, 17253, 17264, 17274, 17285, 17296, 17307, 17317, 17328, 17339, 17349, 17360, 17370, 17381, 17392,
            17402, 17413, 17424, 17434, 17445, 17456, 17466, 17477, 17488, 17498, 17509, 17519, 17530, 17541, 17551, 17562, 17572, 17583, 17594, 17604,
            17615, 17625, 17636, 17647, 17657, 17668, 17678, 17689, 17700, 17710, 17721, 17731, 17742, 17752, 17763, 17774, 17784, 17795, 17805, 17816,
            17826, 17837, 17847, 17858, 17868, 17879, 17889, 17900, 17911, 17921, 17932, 17942, 17953, 17963, 17974, 17984, 17995, 18005, 18016, 18026,
            18037, 18047, 18058, 18068, 18079, 18089, 18100, 18110, 18120, 18131, 18141, 18152, 18162, 18173, 18183, 18194, 18204, 18215, 18225, 18235,
            18246, 18256, 18267, 18277, 18288, 18298, 18308, 18319, 18329, 18340, 18350, 18361, 18371, 18381, 18392, 18402, 18413, 18423, 18433, 18444,
            18454, 18465, 18475, 18485, 18496, 18506, 18516, 18527, 18537, 18547, 18558, 18568, 18579, 18589, 18599, 18610, 18620, 18630, 18641, 18651,
            18661, 18672, 18682, 18692, 18703, 18713, 18723, 18734, 18744, 18754, 18764, 18775, 18785, 18795, 18806, 18816, 18826, 18836, 18847, 18857,
            18867, 18878, 18888, 18898, 18908, 18919, 18929, 18939, 18949, 18960, 18970, 18980, 18990, 19001, 19011, 19021, 19031, 19042, 19052, 19062,
            19072, 19082, 19093, 19103, 19113, 19123, 19133, 19144, 19154, 19164, 19174, 19184, 19195, 19205, 19215, 19225, 19235, 19246, 19256, 19266,
            19276, 19286, 19296, 19306, 19317, 19327, 19337, 19347, 19357, 19367, 19377, 19388, 19398, 19408, 19418, 19428, 19438, 19448, 19458, 19469,
            19479, 19489, 19499, 19509, 19519, 19529, 19539, 19549, 19559, 19570, 19580, 19590, 19600, 19610, 19620, 19630, 19640, 19650, 19660, 19670,
            19680, 19690, 19700, 19710, 19720, 19730, 19740, 19750, 19760, 19771, 19781, 19791, 19801, 19811, 19821, 19831, 19841, 19851, 19861, 19871,
            19881, 19891, 19901, 19911, 19920, 19930, 19940, 19950, 19960, 19970, 19980, 19990, 20000, 20010, 20020, 20030, 20040, 20050, 20060, 20070,
            20080, 20090, 20100, 20110, 20119, 20129, 20139, 20149, 20159, 20169, 20179, 20189, 20199, 20209, 20218, 20228, 20238, 20248, 20258, 20268,
            20278, 20288, 20298, 20307, 20317, 20327, 20337, 20347, 20357, 20366, 20376, 20386, 20396, 20406, 20416, 20425, 20435, 20445, 20455, 20465,
            20475, 20484, 20494, 20504, 20514, 20524, 20533, 20543, 20553, 20563, 20573, 20582, 20592, 20602, 20612, 20621, 20631, 20641, 20651, 20660,
            20670, 20680, 20690, 20699, 20709, 20719, 20729, 20738, 20748, 20758, 20768, 20777, 20787, 20797, 20806, 20816, 20826, 20836, 20845, 20855,
            20865, 20874, 20884, 20894, 20903, 20913, 20923, 20932, 20942, 20952, 20961, 20971, 20981, 20990, 21000, 21010, 21019, 21029, 21039, 21048,
            21058, 21067, 21077, 21087, 21096, 21106, 21115, 21125, 21135, 21144, 21154, 21163, 21173, 21183, 21192, 21202, 21211, 21221, 21231, 21240,
            21250, 21259, 21269, 21278, 21288, 21297, 21307, 21317, 21326, 21336, 21345, 21355, 21364, 21374, 21383, 21393, 21402, 21412, 21421, 21431,
            21440, 21450, 21459, 21469, 21478, 21488, 21497, 21507, 21516, 21526, 21535, 21545, 21554, 21564, 21573, 21583, 21592, 21601, 21611, 21620,
            21630, 21639, 21649, 21658, 21668, 21677, 21686, 21696, 21705, 21715, 21724, 21733, 21743, 21752, 21762, 21771, 21780, 21790, 21799, 21809,
            21818, 21827, 21837, 21846, 21855, 21865, 21874, 21883, 21893, 21902, 21912, 21921, 21930, 21940, 21949, 21958, 21968, 21977, 21986, 21995,
            22005, 22014, 22023, 22033, 22042, 22051, 22061, 22070, 22079, 22088, 22098, 22107, 22116, 22126, 22135, 22144, 22153, 22163, 22172, 22181,
            22190, 22200, 22209, 22218, 22227, 22237, 22246, 22255, 22264, 22273, 22283, 22292, 22301, 22310, 22319, 22329, 22338, 22347, 22356, 22365,
            22375, 22384, 22393, 22402, 22411, 22421, 22430, 22439, 22448, 22457, 22466, 22475, 22485, 22494, 22503, 22512, 22521, 22530, 22539, 22548,
            22558, 22567, 22576, 22585, 22594, 22603, 22612, 22621, 22630, 22639, 22649, 22658, 22667, 22676, 22685, 22694, 22703, 22712, 22721, 22730,
            22739, 22748, 22757, 22766, 22775, 22784, 22793, 22802, 22811, 22821, 22830, 22839, 22848, 22857, 22866, 22875, 22884, 22893, 22902, 22911,
            22919, 22928, 22937, 22946, 22955, 22964, 22973, 22982, 22991, 23000, 23009, 23018, 23027, 23036, 23045, 23054, 23063, 23072, 23081, 23090,
            23098, 23107, 23116, 23125, 23134, 23143, 23152, 23161, 23170, 23179, 23187, 23196, 23205, 23214, 23223, 23232, 23241, 23249, 23258, 23267,
            23276, 23285, 23294, 23303, 23311, 23320, 23329, 23338, 23347, 23355, 23364, 23373, 23382, 23391, 23399, 23408, 23417, 23426, 23435, 23443,
            23452, 23461, 23470, 23479, 23487, 23496, 23505, 23514, 23522, 23531, 23540, 23549, 23557, 23566, 23575, 23583, 23592, 23601, 23610, 23618,
            23627, 23636, 23644, 23653, 23662, 23671, 23679, 23688, 23697, 23705, 23714, 23723, 23731, 23740, 23749, 23757, 23766, 23775, 23783, 23792,
            23800, 23809, 23818, 23826, 23835, 23844, 23852, 23861, 23869, 23878, 23887, 23895, 23904, 23912, 23921, 23930, 23938, 23947, 23955, 23964,
            23973, 23981, 23990, 23998, 24007, 24015, 24024, 24032, 24041, 24049, 24058, 24067, 24075, 24084, 24092, 24101, 24109, 24118, 24126, 24135,
            24143, 24152, 24160, 24169, 24177, 24186, 24194, 24203, 24211, 24219, 24228, 24236, 24245, 24253, 24262, 24270, 24279, 24287, 24295, 24304,
            24312, 24321, 24329, 24338, 24346, 24354, 24363, 24371, 24380, 24388, 24396, 24405, 24413, 24422, 24430, 24438, 24447, 24455, 24463, 24472,
            24480, 24488, 24497, 24505, 24514, 24522, 24530, 24539, 24547, 24555, 24563, 24572, 24580, 24588, 24597, 24605, 24613, 24622, 24630, 24638,
            24646, 24655, 24663, 24671, 24680, 24688, 24696, 24704, 24713, 24721, 24729, 24737, 24746, 24754, 24762, 24770, 24778, 24787, 24795, 24803,
            24811, 24820, 24828, 24836, 24844, 24852, 24861, 24869, 24877, 24885, 24893, 24901, 24910, 24918, 24926, 24934, 24942, 24950, 24958, 24967,
            24975, 24983, 24991, 24999, 25007, 25015, 25024, 25032, 25040, 25048, 25056, 25064, 25072, 25080, 25088, 25096, 25104, 25113, 25121, 25129,
            25137, 25145, 25153, 25161, 25169, 25177, 25185, 25193, 25201, 25209, 25217, 25225, 25233, 25241, 25249, 25257, 25265, 25273, 25281, 25289,
            25297, 25305, 25313, 25321, 25329, 25337, 25345, 25353, 25361, 25369, 25377, 25385, 25393, 25401, 25409, 25417, 25425, 25432, 25440, 25448,
            25456, 25464, 25472, 25480, 25488, 25496, 25504, 25511, 25519, 25527, 25535, 25543, 25551, 25559, 25567, 25574, 25582, 25590, 25598, 25606,
            25614, 25622, 25629, 25637, 25645, 25653, 25661, 25668, 25676, 25684, 25692, 25700, 25707, 25715, 25723, 25731, 25739, 25746, 25754, 25762,
            25770, 25777, 25785, 25793, 25801, 25808, 25816, 25824, 25832, 25839, 25847, 25855, 25863, 25870, 25878, 25886, 25893, 25901, 25909, 25916,
            25924, 25932, 25940, 25947, 25955, 25963, 25970, 25978, 25986, 25993, 26001, 26008, 26016, 26024, 26031, 26039, 26047, 26054, 26062, 26069,
            26077, 26085, 26092, 26100, 26107, 26115, 26123, 26130, 26138, 26145, 26153, 26161, 26168, 26176, 26183, 26191, 26198, 26206, 26213, 26221,
            26228, 26236, 26244, 26251, 26259, 26266, 26274, 26281, 26289, 26296, 26304, 26311, 26319, 26326, 26334, 26341, 26349, 26356, 26363, 26371,
            26378, 26386, 26393, 26401, 26408, 26416, 26423, 26430, 26438, 26445, 26453, 26460, 26468, 26475, 26482, 26490, 26497, 26505, 26512, 26519,
            26527, 26534, 26541, 26549, 26556, 26564, 26571, 26578, 26586, 26593, 26600, 26608, 26615, 26622, 26630, 26637, 26644, 26652, 26659, 26666,
            26673, 26681, 26688, 26695, 26703, 26710, 26717, 26724, 26732, 26739, 26746, 26753, 26761, 26768, 26775, 26782, 26790, 26797, 26804, 26811,
            26819, 26826, 26833, 26840, 26847, 26855, 26862, 26869, 26876, 26883, 26891, 26898, 26905, 26912, 26919, 26926, 26934, 26941, 26948, 26955,
            26962, 26969, 26977, 26984, 26991, 26998, 27005, 27012, 27019, 27026, 27033, 27041, 27048, 27055, 27062, 27069, 27076, 27083, 27090, 27097,
            27104, 27111, 27118, 27125, 27132, 27140, 27147, 27154, 27161, 27168, 27175, 27182, 27189, 27196, 27203, 27210, 27217, 27224, 27231, 27238,
            27245, 27252, 27259, 27266, 27273, 27280, 27286, 27293, 27300, 27307, 27314, 27321, 27328, 27335, 27342, 27349, 27356, 27363, 27370, 27377,
            27384, 27390, 27397, 27404, 27411, 27418, 27425, 27432, 27439, 27445, 27452, 27459, 27466, 27473, 27480, 27487, 27493, 27500, 27507, 27514,
            27521, 27528, 27534, 27541, 27548, 27555, 27562, 27568, 27575, 27582, 27589, 27595, 27602, 27609, 27616, 27623, 27629, 27636, 27643, 27650,
            27656, 27663, 27670, 27677, 27683, 27690, 27697, 27703, 27710, 27717, 27724, 27730, 27737, 27744, 27750, 27757, 27764, 27770, 27777, 27784,
            27790, 27797, 27804, 27810, 27817, 27824, 27830, 27837, 27843, 27850, 27857, 27863, 27870, 27876, 27883, 27890, 27896, 27903, 27909, 27916,
            27923, 27929, 27936, 27942, 27949, 27955, 27962, 27969, 27975, 27982, 27988, 27995, 28001, 28008, 28014, 28021, 28027, 28034, 28040, 28047,
            28053, 28060, 28066, 28073, 28079, 28086, 28092, 28099, 28105, 28112, 28118, 28124, 28131, 28137, 28144, 28150, 28157, 28163, 28170, 28176,
            28182, 28189, 28195, 28202, 28208, 28214, 28221, 28227, 28234, 28240, 28246, 28253, 28259, 28265, 28272, 28278, 28284, 28291, 28297, 28303,
            28310, 28316, 28322, 28329, 28335, 28341, 28348, 28354, 28360, 28367, 28373, 28379, 28385, 28392, 28398, 28404, 28410, 28417, 28423, 28429,
            28435, 28442, 28448, 28454, 28460, 28467, 28473, 28479, 28485, 28491, 28498, 28504, 28510, 28516, 28522, 28529, 28535, 28541, 28547, 28553,
            28559, 28566, 28572, 28578, 28584, 28590, 28596, 28603, 28609, 28615, 28621, 28627, 28633, 28639, 28645, 28651, 28658, 28664, 28670, 28676,
            28682, 28688, 28694, 28700, 28706, 28712, 28718, 28724, 28730, 28736, 28742, 28748, 28754, 28760, 28767, 28773, 28779, 28785, 28791, 28797,
            28803, 28809, 28815, 28820, 28826, 28832, 28838, 28844, 28850, 28856, 28862, 28868, 28874, 28880, 28886, 28892, 28898, 28904, 28910, 28916,
            28922, 28927, 28933, 28939, 28945, 28951, 28957, 28963, 28969, 28975, 28980, 28986, 28992, 28998, 29004, 29010, 29015, 29021, 29027, 29033,
            29039, 29045, 29050, 29056, 29062, 29068, 29074, 29079, 29085, 29091, 29097, 29103, 29108, 29114, 29120, 29126, 29131, 29137, 29143, 29149,
            29154, 29160, 29166, 29172, 29177, 29183, 29189, 29194, 29200, 29206, 29212, 29217, 29223, 29229, 29234, 29240, 29246, 29251, 29257, 29263,
            29268, 29274, 29280, 29285, 29291, 29296, 29302, 29308, 29313, 29319, 29325, 29330, 29336, 29341, 29347, 29352, 29358, 29364, 29369, 29375,
            29380, 29386, 29391, 29397, 29403, 29408, 29414, 29419, 29425, 29430, 29436, 29441, 29447, 29452, 29458, 29463, 29469, 29474, 29480, 29485,
            29491, 29496, 29502, 29507, 29513, 29518, 29524, 29529, 29534, 29540, 29545, 29551, 29556, 29562, 29567, 29572, 29578, 29583, 29589, 29594,
            29599, 29605, 29610, 29616, 29621, 29626, 29632, 29637, 29642, 29648, 29653, 29658, 29664, 29669, 29674, 29680, 29685, 29690, 29696, 29701,
            29706, 29712, 29717, 29722, 29728, 29733, 29738, 29743, 29749, 29754, 29759, 29764, 29770, 29775, 29780, 29785, 29791, 29796, 29801, 29806,
            29812, 29817, 29822, 29827, 29832, 29838, 29843, 29848, 29853, 29858, 29863, 29869, 29874, 29879, 29884, 29889, 29894, 29900, 29905, 29910,
            29915, 29920, 29925, 29930, 29935, 29941, 29946, 29951, 29956, 29961, 29966, 29971, 29976, 29981, 29986, 29991, 29996, 30002, 30007, 30012,
            30017, 30022, 30027, 30032, 30037, 30042, 30047, 30052, 30057, 30062, 30067, 30072, 30077, 30082, 30087, 30092, 30097, 30102, 30107, 30112,
            30117, 30122, 30126, 30131, 30136, 30141, 30146, 30151, 30156, 30161, 30166, 30171, 30176, 30181, 30185, 30190, 30195, 30200, 30205, 30210,
            30215, 30220, 30224, 30229, 30234, 30239, 30244, 30249, 30253, 30258, 30263, 30268, 30273, 30278, 30282, 30287, 30292, 30297, 30301, 30306,
            30311, 30316, 30321, 30325, 30330, 30335, 30340, 30344, 30349, 30354, 30359, 30363, 30368, 30373, 30377, 30382, 30387, 30392, 30396, 30401,
            30406, 30410, 30415, 30420, 30424, 30429, 30434, 30438, 30443, 30448, 30452, 30457, 30462, 30466, 30471, 30475, 30480, 30485, 30489, 30494,
            30498, 30503, 30508, 30512, 30517, 30521, 30526, 30530, 30535, 30540, 30544, 30549, 30553, 30558, 30562, 30567, 30571, 30576, 30580, 30585,
            30589, 30594, 30598, 30603, 30607, 30612, 30616, 30621, 30625, 30630, 30634, 30639, 30643, 30648, 30652, 30656, 30661, 30665, 30670, 30674,
            30679, 30683, 30687, 30692, 30696, 30701, 30705, 30709, 30714, 30718, 30723, 30727, 30731, 30736, 30740, 30744, 30749, 30753, 30757, 30762,
            30766, 30770, 30775, 30779, 30783, 30788, 30792, 30796, 30800, 30805, 30809, 30813, 30818, 30822, 30826, 30830, 30835, 30839, 30843, 30847,
            30852, 30856, 30860, 30864, 30868, 30873, 30877, 30881, 30885, 30889, 30894, 30898, 30902, 30906, 30910, 30915, 30919, 30923, 30927, 30931,
            30935, 30939, 30944, 30948, 30952, 30956, 30960, 30964, 30968, 30972, 30977, 30981, 30985, 30989, 30993, 30997, 31001, 31005, 31009, 31013,
            31017, 31021, 31025, 31029, 31033, 31037, 31041, 31045, 31050, 31054, 31058, 31062, 31066, 31070, 31074, 31078, 31081, 31085, 31089, 31093,
            31097, 31101, 31105, 31109, 31113, 31117, 31121, 31125, 31129, 31133, 31137, 31141, 31145, 31148, 31152, 31156, 31160, 31164, 31168, 31172,
            31176, 31180, 31183, 31187, 31191, 31195, 31199, 31203, 31206, 31210, 31214, 31218, 31222, 31226, 31229, 31233, 31237, 31241, 31245, 31248,
            31252, 31256, 31260, 31263, 31267, 31271, 31275, 31278, 31282, 31286, 31290, 31293, 31297, 31301, 31305, 31308, 31312, 31316, 31319, 31323,
            31327, 31330, 31334, 31338, 31341, 31345, 31349, 31352, 31356, 31360, 31363, 31367, 31371, 31374, 31378, 31381, 31385, 31389, 31392, 31396,
            31399, 31403, 31407, 31410, 31414, 31417, 31421, 31425, 31428, 31432, 31435, 31439, 31442, 31446, 31449, 31453, 31456, 31460, 31463, 31467,
            31470, 31474, 31477, 31481, 31484, 31488, 31491, 31495, 31498, 31502, 31505, 31509, 31512, 31516, 31519, 31522, 31526, 31529, 31533, 31536,
            31539, 31543, 31546, 31550, 31553, 31556, 31560, 31563, 31567, 31570, 31573, 31577, 31580, 31583, 31587, 31590, 31593, 31597, 31600, 31603,
            31607, 31610, 31613, 31617, 31620, 31623, 31626, 31630, 31633, 31636, 31640, 31643, 31646, 31649, 31653, 31656, 31659, 31662, 31666, 31669,
            31672, 31675, 31678, 31682, 31685, 31688, 31691, 31694, 31698, 31701, 31704, 31707, 31710, 31714, 31717, 31720, 31723, 31726, 31729, 31732,
            31736, 31739, 31742, 31745, 31748, 31751, 31754, 31757, 31760, 31764, 31767, 31770, 31773, 31776, 31779, 31782, 31785, 31788, 31791, 31794,
            31797, 31800, 31803, 31806, 31809, 31812, 31815, 31818, 31821, 31824, 31827, 31830, 31833, 31836, 31839, 31842, 31845, 31848, 31851, 31854,
            31857, 31860, 31863, 31866, 31869, 31872, 31874, 31877, 31880, 31883, 31886, 31889, 31892, 31895, 31898, 31901, 31903, 31906, 31909, 31912,
            31915, 31918, 31920, 31923, 31926, 31929, 31932, 31935, 31937, 31940, 31943, 31946, 31949, 31951, 31954, 31957, 31960, 31963, 31965, 31968,
            31971, 31974, 31976, 31979, 31982, 31985, 31987, 31990, 31993, 31995, 31998, 32001, 32004, 32006, 32009, 32012, 32014, 32017, 32020, 32022,
            32025, 32028, 32030, 32033, 32036, 32038, 32041, 32043, 32046, 32049, 32051, 32054, 32057, 32059, 32062, 32064, 32067, 32069, 32072, 32075,
            32077, 32080, 32082, 32085, 32087, 32090, 32092, 32095, 32098, 32100, 32103, 32105, 32108, 32110, 32113, 32115, 32118, 32120, 32123, 32125,
            32128, 32130, 32132, 32135, 32137, 32140, 32142, 32145, 32147, 32150, 32152, 32154, 32157, 32159, 32162, 32164, 32166, 32169, 32171, 32174,
            32176, 32178, 32181, 32183, 32185, 32188, 32190, 32193, 32195, 32197, 32200, 32202, 32204, 32206, 32209, 32211, 32213, 32216, 32218, 32220,
            32223, 32225, 32227, 32229, 32232, 32234, 32236, 32238, 32241, 32243, 32245, 32247, 32250, 32252, 32254, 32256, 32258, 32261, 32263, 32265,
            32267, 32269, 32272, 32274, 32276, 32278, 32280, 32282, 32285, 32287, 32289, 32291, 32293, 32295, 32297, 32300, 32302, 32304, 32306, 32308,
            32310, 32312, 32314, 32316, 32318, 32320, 32322, 32325, 32327, 32329, 32331, 32333, 32335, 32337, 32339, 32341, 32343, 32345, 32347, 32349,
            32351, 32353, 32355, 32357, 32359, 32361, 32363, 32365, 32367, 32369, 32371, 32373, 32375, 32376, 32378, 32380, 32382, 32384, 32386, 32388,
            32390, 32392, 32394, 32396, 32397, 32399, 32401, 32403, 32405, 32407, 32409, 32410, 32412, 32414, 32416, 32418, 32420, 32422, 32423, 32425,
            32427, 32429, 32431, 32432, 32434, 32436, 32438, 32439, 32441, 32443, 32445, 32447, 32448, 32450, 32452, 32453, 32455, 32457, 32459, 32460,
            32462, 32464, 32466, 32467, 32469, 32471, 32472, 32474, 32476, 32477, 32479, 32481, 32482, 32484, 32486, 32487, 32489, 32490, 32492, 32494,
            32495, 32497, 32499, 32500, 32502, 32503, 32505, 32507, 32508, 32510, 32511, 32513, 32514, 32516, 32517, 32519, 32521, 32522, 32524, 32525,
            32527, 32528, 32530, 32531, 32533, 32534, 32536, 32537, 32539, 32540, 32542, 32543, 32545, 32546, 32547, 32549, 32550, 32552, 32553, 32555,
            32556, 32558, 32559, 32560, 32562, 32563, 32565, 32566, 32567, 32569, 32570, 32571, 32573, 32574, 32576, 32577, 32578, 32580, 32581, 32582,
            32584, 32585, 32586, 32588, 32589, 32590, 32592, 32593, 32594, 32595, 32597, 32598, 32599, 32600, 32602, 32603, 32604, 32605, 32607, 32608,
            32609, 32610, 32612, 32613, 32614, 32615, 32617, 32618, 32619, 32620, 32621, 32622, 32624, 32625, 32626, 32627, 32628, 32629, 32631, 32632,
            32633, 32634, 32635, 32636, 32637, 32639, 32640, 32641, 32642, 32643, 32644, 32645, 32646, 32647, 32648, 32649, 32650, 32652, 32653, 32654,
            32655, 32656, 32657, 32658, 32659, 32660, 32661, 32662, 32663, 32664, 32665, 32666, 32667, 32668, 32669, 32670, 32671, 32672, 32673, 32674,
            32674, 32675, 32676, 32677, 32678, 32679, 32680, 32681, 32682, 32683, 32684, 32685, 32685, 32686, 32687, 32688, 32689, 32690, 32691, 32692,
            32692, 32693, 32694, 32695, 32696, 32697, 32697, 32698, 32699, 32700, 32701, 32701, 32702, 32703, 32704, 32705, 32705, 32706, 32707, 32708,
            32708, 32709, 32710, 32711, 32711, 32712, 32713, 32714, 32714, 32715, 32716, 32716, 32717, 32718, 32718, 32719, 32720, 32720, 32721, 32722,
            32722, 32723, 32724, 32724, 32725, 32726, 32726, 32727, 32728, 32728, 32729, 32729, 32730, 32731, 32731, 32732, 32732, 32733, 32733, 32734,
            32735, 32735, 32736, 32736, 32737, 32737, 32738, 32738, 32739, 32739, 32740, 32740, 32741, 32741, 32742, 32742, 32743, 32743, 32744, 32744,
            32745, 32745, 32746, 32746, 32747, 32747, 32747, 32748, 32748, 32749, 32749, 32750, 32750, 32750, 32751, 32751, 32752, 32752, 32752, 32753,
            32753, 32753, 32754, 32754, 32755, 32755, 32755, 32756, 32756, 32756, 32757, 32757, 32757, 32757, 32758, 32758, 32758, 32759, 32759, 32759,
            32759, 32760, 32760, 32760, 32760, 32761, 32761, 32761, 32761, 32762, 32762, 32762, 32762, 32763, 32763, 32763, 32763, 32763, 32764, 32764,
            32764, 32764, 32764, 32764, 32765, 32765, 32765, 32765, 32765, 32765, 32765, 32765, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
            32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
        };
        // clang-format on
    }

    int32_t integerSinePrecisionHigh(uint16_t direction, int32_t magnitude)
    {
        // Build the full sine wave from the quarter wave by subtraction of direction/magnitude
        const auto sineIndex = ((direction & (1 << 12)) ? -direction : direction) & 0xFFF;
        const auto value = (direction & (1 << 13)) ? -Data::kQuarterSine[sineIndex] : Data::kQuarterSine[sineIndex];
        return value * magnitude / 0x8000;
    }

    int32_t integerCosinePrecisionHigh(uint16_t direction, int32_t magnitude)
    {
        // Cosine is Sine plus pi/2
        return integerSinePrecisionHigh(direction + kDirectionPrecisionHigh / 4, magnitude);
    }
}
