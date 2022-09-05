#include "PaintTileDecorations.h"
#include "../Config.h"
#include "../GameState.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco::Paint
{
    static Interop::loco_global<int8_t[2 * 44], 0x004F86B4> _4F86B4;

    static constexpr std::array<std::array<uint32_t, 256>, 3> kHeightMarkerImages = {
        // MicroZ Units
        std::array<uint32_t, 256>{
            ImageIds::height_marker_neg_128_u,
            ImageIds::height_marker_neg_127_u,
            ImageIds::height_marker_neg_126_u,
            ImageIds::height_marker_neg_125_u,
            ImageIds::height_marker_neg_124_u,
            ImageIds::height_marker_neg_123_u,
            ImageIds::height_marker_neg_122_u,
            ImageIds::height_marker_neg_121_u,
            ImageIds::height_marker_neg_120_u,
            ImageIds::height_marker_neg_119_u,
            ImageIds::height_marker_neg_118_u,
            ImageIds::height_marker_neg_117_u,
            ImageIds::height_marker_neg_116_u,
            ImageIds::height_marker_neg_115_u,
            ImageIds::height_marker_neg_114_u,
            ImageIds::height_marker_neg_113_u,
            ImageIds::height_marker_neg_112_u,
            ImageIds::height_marker_neg_111_u,
            ImageIds::height_marker_neg_110_u,
            ImageIds::height_marker_neg_109_u,
            ImageIds::height_marker_neg_108_u,
            ImageIds::height_marker_neg_107_u,
            ImageIds::height_marker_neg_106_u,
            ImageIds::height_marker_neg_105_u,
            ImageIds::height_marker_neg_104_u,
            ImageIds::height_marker_neg_103_u,
            ImageIds::height_marker_neg_102_u,
            ImageIds::height_marker_neg_101_u,
            ImageIds::height_marker_neg_100_u,
            ImageIds::height_marker_neg_99_u,
            ImageIds::height_marker_neg_98_u,
            ImageIds::height_marker_neg_97_u,
            ImageIds::height_marker_neg_96_u,
            ImageIds::height_marker_neg_95_u,
            ImageIds::height_marker_neg_94_u,
            ImageIds::height_marker_neg_93_u,
            ImageIds::height_marker_neg_92_u,
            ImageIds::height_marker_neg_91_u,
            ImageIds::height_marker_neg_90_u,
            ImageIds::height_marker_neg_89_u,
            ImageIds::height_marker_neg_88_u,
            ImageIds::height_marker_neg_87_u,
            ImageIds::height_marker_neg_86_u,
            ImageIds::height_marker_neg_85_u,
            ImageIds::height_marker_neg_84_u,
            ImageIds::height_marker_neg_83_u,
            ImageIds::height_marker_neg_82_u,
            ImageIds::height_marker_neg_81_u,
            ImageIds::height_marker_neg_80_u,
            ImageIds::height_marker_neg_79_u,
            ImageIds::height_marker_neg_78_u,
            ImageIds::height_marker_neg_77_u,
            ImageIds::height_marker_neg_76_u,
            ImageIds::height_marker_neg_75_u,
            ImageIds::height_marker_neg_74_u,
            ImageIds::height_marker_neg_73_u,
            ImageIds::height_marker_neg_72_u,
            ImageIds::height_marker_neg_71_u,
            ImageIds::height_marker_neg_70_u,
            ImageIds::height_marker_neg_69_u,
            ImageIds::height_marker_neg_68_u,
            ImageIds::height_marker_neg_67_u,
            ImageIds::height_marker_neg_66_u,
            ImageIds::height_marker_neg_65_u,
            ImageIds::height_marker_neg_64_u,
            ImageIds::height_marker_neg_63_u,
            ImageIds::height_marker_neg_62_u,
            ImageIds::height_marker_neg_61_u,
            ImageIds::height_marker_neg_60_u,
            ImageIds::height_marker_neg_59_u,
            ImageIds::height_marker_neg_58_u,
            ImageIds::height_marker_neg_57_u,
            ImageIds::height_marker_neg_56_u,
            ImageIds::height_marker_neg_55_u,
            ImageIds::height_marker_neg_54_u,
            ImageIds::height_marker_neg_53_u,
            ImageIds::height_marker_neg_52_u,
            ImageIds::height_marker_neg_51_u,
            ImageIds::height_marker_neg_50_u,
            ImageIds::height_marker_neg_49_u,
            ImageIds::height_marker_neg_48_u,
            ImageIds::height_marker_neg_47_u,
            ImageIds::height_marker_neg_46_u,
            ImageIds::height_marker_neg_45_u,
            ImageIds::height_marker_neg_44_u,
            ImageIds::height_marker_neg_43_u,
            ImageIds::height_marker_neg_42_u,
            ImageIds::height_marker_neg_41_u,
            ImageIds::height_marker_neg_40_u,
            ImageIds::height_marker_neg_39_u,
            ImageIds::height_marker_neg_38_u,
            ImageIds::height_marker_neg_37_u,
            ImageIds::height_marker_neg_36_u,
            ImageIds::height_marker_neg_35_u,
            ImageIds::height_marker_neg_34_u,
            ImageIds::height_marker_neg_33_u,
            ImageIds::height_marker_neg_32_u,
            ImageIds::height_marker_neg_31_u,
            ImageIds::height_marker_neg_30_u,
            ImageIds::height_marker_neg_29_u,
            ImageIds::height_marker_neg_28_u,
            ImageIds::height_marker_neg_27_u,
            ImageIds::height_marker_neg_26_u,
            ImageIds::height_marker_neg_25_u,
            ImageIds::height_marker_neg_24_u,
            ImageIds::height_marker_neg_23_u,
            ImageIds::height_marker_neg_22_u,
            ImageIds::height_marker_neg_21_u,
            ImageIds::height_marker_neg_20_u,
            ImageIds::height_marker_neg_19_u,
            ImageIds::height_marker_neg_18_u,
            ImageIds::height_marker_neg_17_u,
            ImageIds::height_marker_neg_16_u,
            ImageIds::height_marker_neg_15_u,
            ImageIds::height_marker_neg_14_u,
            ImageIds::height_marker_neg_13_u,
            ImageIds::height_marker_neg_12_u,
            ImageIds::height_marker_neg_11_u,
            ImageIds::height_marker_neg_10_u,
            ImageIds::height_marker_neg_9_u,
            ImageIds::height_marker_neg_8_u,
            ImageIds::height_marker_neg_7_u,
            ImageIds::height_marker_neg_6_u,
            ImageIds::height_marker_neg_5_u,
            ImageIds::height_marker_neg_4_u,
            ImageIds::height_marker_neg_3_u,
            ImageIds::height_marker_neg_2_u,
            ImageIds::height_marker_neg_1_u,
            ImageIds::height_marker_0_u,
            ImageIds::height_marker_1_u,
            ImageIds::height_marker_2_u,
            ImageIds::height_marker_3_u,
            ImageIds::height_marker_4_u,
            ImageIds::height_marker_5_u,
            ImageIds::height_marker_6_u,
            ImageIds::height_marker_7_u,
            ImageIds::height_marker_8_u,
            ImageIds::height_marker_9_u,
            ImageIds::height_marker_10_u,
            ImageIds::height_marker_11_u,
            ImageIds::height_marker_12_u,
            ImageIds::height_marker_13_u,
            ImageIds::height_marker_14_u,
            ImageIds::height_marker_15_u,
            ImageIds::height_marker_16_u,
            ImageIds::height_marker_17_u,
            ImageIds::height_marker_18_u,
            ImageIds::height_marker_19_u,
            ImageIds::height_marker_20_u,
            ImageIds::height_marker_21_u,
            ImageIds::height_marker_22_u,
            ImageIds::height_marker_23_u,
            ImageIds::height_marker_24_u,
            ImageIds::height_marker_25_u,
            ImageIds::height_marker_26_u,
            ImageIds::height_marker_27_u,
            ImageIds::height_marker_28_u,
            ImageIds::height_marker_29_u,
            ImageIds::height_marker_30_u,
            ImageIds::height_marker_31_u,
            ImageIds::height_marker_32_u,
            ImageIds::height_marker_33_u,
            ImageIds::height_marker_34_u,
            ImageIds::height_marker_35_u,
            ImageIds::height_marker_36_u,
            ImageIds::height_marker_37_u,
            ImageIds::height_marker_38_u,
            ImageIds::height_marker_39_u,
            ImageIds::height_marker_40_u,
            ImageIds::height_marker_41_u,
            ImageIds::height_marker_42_u,
            ImageIds::height_marker_43_u,
            ImageIds::height_marker_44_u,
            ImageIds::height_marker_45_u,
            ImageIds::height_marker_46_u,
            ImageIds::height_marker_47_u,
            ImageIds::height_marker_48_u,
            ImageIds::height_marker_49_u,
            ImageIds::height_marker_50_u,
            ImageIds::height_marker_51_u,
            ImageIds::height_marker_52_u,
            ImageIds::height_marker_53_u,
            ImageIds::height_marker_54_u,
            ImageIds::height_marker_55_u,
            ImageIds::height_marker_56_u,
            ImageIds::height_marker_57_u,
            ImageIds::height_marker_58_u,
            ImageIds::height_marker_59_u,
            ImageIds::height_marker_60_u,
            ImageIds::height_marker_61_u,
            ImageIds::height_marker_62_u,
            ImageIds::height_marker_63_u,
            ImageIds::height_marker_64_u,
            ImageIds::height_marker_65_u,
            ImageIds::height_marker_66_u,
            ImageIds::height_marker_67_u,
            ImageIds::height_marker_68_u,
            ImageIds::height_marker_69_u,
            ImageIds::height_marker_70_u,
            ImageIds::height_marker_71_u,
            ImageIds::height_marker_72_u,
            ImageIds::height_marker_73_u,
            ImageIds::height_marker_74_u,
            ImageIds::height_marker_75_u,
            ImageIds::height_marker_76_u,
            ImageIds::height_marker_77_u,
            ImageIds::height_marker_78_u,
            ImageIds::height_marker_79_u,
            ImageIds::height_marker_80_u,
            ImageIds::height_marker_81_u,
            ImageIds::height_marker_82_u,
            ImageIds::height_marker_83_u,
            ImageIds::height_marker_84_u,
            ImageIds::height_marker_85_u,
            ImageIds::height_marker_86_u,
            ImageIds::height_marker_87_u,
            ImageIds::height_marker_88_u,
            ImageIds::height_marker_89_u,
            ImageIds::height_marker_90_u,
            ImageIds::height_marker_91_u,
            ImageIds::height_marker_92_u,
            ImageIds::height_marker_93_u,
            ImageIds::height_marker_94_u,
            ImageIds::height_marker_95_u,
            ImageIds::height_marker_96_u,
            ImageIds::height_marker_97_u,
            ImageIds::height_marker_98_u,
            ImageIds::height_marker_99_u,
            ImageIds::height_marker_100_u,
            ImageIds::height_marker_101_u,
            ImageIds::height_marker_102_u,
            ImageIds::height_marker_103_u,
            ImageIds::height_marker_104_u,
            ImageIds::height_marker_105_u,
            ImageIds::height_marker_106_u,
            ImageIds::height_marker_107_u,
            ImageIds::height_marker_108_u,
            ImageIds::height_marker_109_u,
            ImageIds::height_marker_110_u,
            ImageIds::height_marker_111_u,
            ImageIds::height_marker_112_u,
            ImageIds::height_marker_113_u,
            ImageIds::height_marker_114_u,
            ImageIds::height_marker_115_u,
            ImageIds::height_marker_116_u,
            ImageIds::height_marker_117_u,
            ImageIds::height_marker_118_u,
            ImageIds::height_marker_119_u,
            ImageIds::height_marker_120_u,
            ImageIds::height_marker_121_u,
            ImageIds::height_marker_122_u,
            ImageIds::height_marker_123_u,
            ImageIds::height_marker_124_u,
            ImageIds::height_marker_125_u,
            ImageIds::height_marker_126_u,
            ImageIds::height_marker_127_u,
        },
        // Imperial
        std::array<uint32_t, 256>{
            ImageIds::height_marker_neg_128_i,
            ImageIds::height_marker_neg_127_i,
            ImageIds::height_marker_neg_126_i,
            ImageIds::height_marker_neg_125_i,
            ImageIds::height_marker_neg_124_i,
            ImageIds::height_marker_neg_123_i,
            ImageIds::height_marker_neg_122_i,
            ImageIds::height_marker_neg_121_i,
            ImageIds::height_marker_neg_120_i,
            ImageIds::height_marker_neg_119_i,
            ImageIds::height_marker_neg_118_i,
            ImageIds::height_marker_neg_117_i,
            ImageIds::height_marker_neg_116_i,
            ImageIds::height_marker_neg_115_i,
            ImageIds::height_marker_neg_114_i,
            ImageIds::height_marker_neg_113_i,
            ImageIds::height_marker_neg_112_i,
            ImageIds::height_marker_neg_111_i,
            ImageIds::height_marker_neg_110_i,
            ImageIds::height_marker_neg_109_i,
            ImageIds::height_marker_neg_108_i,
            ImageIds::height_marker_neg_107_i,
            ImageIds::height_marker_neg_106_i,
            ImageIds::height_marker_neg_105_i,
            ImageIds::height_marker_neg_104_i,
            ImageIds::height_marker_neg_103_i,
            ImageIds::height_marker_neg_102_i,
            ImageIds::height_marker_neg_101_i,
            ImageIds::height_marker_neg_100_i,
            ImageIds::height_marker_neg_99_i,
            ImageIds::height_marker_neg_98_i,
            ImageIds::height_marker_neg_97_i,
            ImageIds::height_marker_neg_96_i,
            ImageIds::height_marker_neg_95_i,
            ImageIds::height_marker_neg_94_i,
            ImageIds::height_marker_neg_93_i,
            ImageIds::height_marker_neg_92_i,
            ImageIds::height_marker_neg_91_i,
            ImageIds::height_marker_neg_90_i,
            ImageIds::height_marker_neg_89_i,
            ImageIds::height_marker_neg_88_i,
            ImageIds::height_marker_neg_87_i,
            ImageIds::height_marker_neg_86_i,
            ImageIds::height_marker_neg_85_i,
            ImageIds::height_marker_neg_84_i,
            ImageIds::height_marker_neg_83_i,
            ImageIds::height_marker_neg_82_i,
            ImageIds::height_marker_neg_81_i,
            ImageIds::height_marker_neg_80_i,
            ImageIds::height_marker_neg_79_i,
            ImageIds::height_marker_neg_78_i,
            ImageIds::height_marker_neg_77_i,
            ImageIds::height_marker_neg_76_i,
            ImageIds::height_marker_neg_75_i,
            ImageIds::height_marker_neg_74_i,
            ImageIds::height_marker_neg_73_i,
            ImageIds::height_marker_neg_72_i,
            ImageIds::height_marker_neg_71_i,
            ImageIds::height_marker_neg_70_i,
            ImageIds::height_marker_neg_69_i,
            ImageIds::height_marker_neg_68_i,
            ImageIds::height_marker_neg_67_i,
            ImageIds::height_marker_neg_66_i,
            ImageIds::height_marker_neg_65_i,
            ImageIds::height_marker_neg_64_i,
            ImageIds::height_marker_neg_63_i,
            ImageIds::height_marker_neg_62_i,
            ImageIds::height_marker_neg_61_i,
            ImageIds::height_marker_neg_60_i,
            ImageIds::height_marker_neg_59_i,
            ImageIds::height_marker_neg_58_i,
            ImageIds::height_marker_neg_57_i,
            ImageIds::height_marker_neg_56_i,
            ImageIds::height_marker_neg_55_i,
            ImageIds::height_marker_neg_54_i,
            ImageIds::height_marker_neg_53_i,
            ImageIds::height_marker_neg_52_i,
            ImageIds::height_marker_neg_51_i,
            ImageIds::height_marker_neg_50_i,
            ImageIds::height_marker_neg_49_i,
            ImageIds::height_marker_neg_48_i,
            ImageIds::height_marker_neg_47_i,
            ImageIds::height_marker_neg_46_i,
            ImageIds::height_marker_neg_45_i,
            ImageIds::height_marker_neg_44_i,
            ImageIds::height_marker_neg_43_i,
            ImageIds::height_marker_neg_42_i,
            ImageIds::height_marker_neg_41_i,
            ImageIds::height_marker_neg_40_i,
            ImageIds::height_marker_neg_39_i,
            ImageIds::height_marker_neg_38_i,
            ImageIds::height_marker_neg_37_i,
            ImageIds::height_marker_neg_36_i,
            ImageIds::height_marker_neg_35_i,
            ImageIds::height_marker_neg_34_i,
            ImageIds::height_marker_neg_33_i,
            ImageIds::height_marker_neg_32_i,
            ImageIds::height_marker_neg_31_i,
            ImageIds::height_marker_neg_30_i,
            ImageIds::height_marker_neg_29_i,
            ImageIds::height_marker_neg_28_i,
            ImageIds::height_marker_neg_27_i,
            ImageIds::height_marker_neg_26_i,
            ImageIds::height_marker_neg_25_i,
            ImageIds::height_marker_neg_24_i,
            ImageIds::height_marker_neg_23_i,
            ImageIds::height_marker_neg_22_i,
            ImageIds::height_marker_neg_21_i,
            ImageIds::height_marker_neg_20_i,
            ImageIds::height_marker_neg_19_i,
            ImageIds::height_marker_neg_18_i,
            ImageIds::height_marker_neg_17_i,
            ImageIds::height_marker_neg_16_i,
            ImageIds::height_marker_neg_15_i,
            ImageIds::height_marker_neg_14_i,
            ImageIds::height_marker_neg_13_i,
            ImageIds::height_marker_neg_12_i,
            ImageIds::height_marker_neg_11_i,
            ImageIds::height_marker_neg_10_i,
            ImageIds::height_marker_neg_9_i,
            ImageIds::height_marker_neg_8_i,
            ImageIds::height_marker_neg_7_i,
            ImageIds::height_marker_neg_6_i,
            ImageIds::height_marker_neg_5_i,
            ImageIds::height_marker_neg_4_i,
            ImageIds::height_marker_neg_3_i,
            ImageIds::height_marker_neg_2_i,
            ImageIds::height_marker_neg_1_i,
            ImageIds::height_marker_0_i,
            ImageIds::height_marker_1_i,
            ImageIds::height_marker_2_i,
            ImageIds::height_marker_3_i,
            ImageIds::height_marker_4_i,
            ImageIds::height_marker_5_i,
            ImageIds::height_marker_6_i,
            ImageIds::height_marker_7_i,
            ImageIds::height_marker_8_i,
            ImageIds::height_marker_9_i,
            ImageIds::height_marker_10_i,
            ImageIds::height_marker_11_i,
            ImageIds::height_marker_12_i,
            ImageIds::height_marker_13_i,
            ImageIds::height_marker_14_i,
            ImageIds::height_marker_15_i,
            ImageIds::height_marker_16_i,
            ImageIds::height_marker_17_i,
            ImageIds::height_marker_18_i,
            ImageIds::height_marker_19_i,
            ImageIds::height_marker_20_i,
            ImageIds::height_marker_21_i,
            ImageIds::height_marker_22_i,
            ImageIds::height_marker_23_i,
            ImageIds::height_marker_24_i,
            ImageIds::height_marker_25_i,
            ImageIds::height_marker_26_i,
            ImageIds::height_marker_27_i,
            ImageIds::height_marker_28_i,
            ImageIds::height_marker_29_i,
            ImageIds::height_marker_30_i,
            ImageIds::height_marker_31_i,
            ImageIds::height_marker_32_i,
            ImageIds::height_marker_33_i,
            ImageIds::height_marker_34_i,
            ImageIds::height_marker_35_i,
            ImageIds::height_marker_36_i,
            ImageIds::height_marker_37_i,
            ImageIds::height_marker_38_i,
            ImageIds::height_marker_39_i,
            ImageIds::height_marker_40_i,
            ImageIds::height_marker_41_i,
            ImageIds::height_marker_42_i,
            ImageIds::height_marker_43_i,
            ImageIds::height_marker_44_i,
            ImageIds::height_marker_45_i,
            ImageIds::height_marker_46_i,
            ImageIds::height_marker_47_i,
            ImageIds::height_marker_48_i,
            ImageIds::height_marker_49_i,
            ImageIds::height_marker_50_i,
            ImageIds::height_marker_51_i,
            ImageIds::height_marker_52_i,
            ImageIds::height_marker_53_i,
            ImageIds::height_marker_54_i,
            ImageIds::height_marker_55_i,
            ImageIds::height_marker_56_i,
            ImageIds::height_marker_57_i,
            ImageIds::height_marker_58_i,
            ImageIds::height_marker_59_i,
            ImageIds::height_marker_60_i,
            ImageIds::height_marker_61_i,
            ImageIds::height_marker_62_i,
            ImageIds::height_marker_63_i,
            ImageIds::height_marker_64_i,
            ImageIds::height_marker_65_i,
            ImageIds::height_marker_66_i,
            ImageIds::height_marker_67_i,
            ImageIds::height_marker_68_i,
            ImageIds::height_marker_69_i,
            ImageIds::height_marker_70_i,
            ImageIds::height_marker_71_i,
            ImageIds::height_marker_72_i,
            ImageIds::height_marker_73_i,
            ImageIds::height_marker_74_i,
            ImageIds::height_marker_75_i,
            ImageIds::height_marker_76_i,
            ImageIds::height_marker_77_i,
            ImageIds::height_marker_78_i,
            ImageIds::height_marker_79_i,
            ImageIds::height_marker_80_i,
            ImageIds::height_marker_81_i,
            ImageIds::height_marker_82_i,
            ImageIds::height_marker_83_i,
            ImageIds::height_marker_84_i,
            ImageIds::height_marker_85_i,
            ImageIds::height_marker_86_i,
            ImageIds::height_marker_87_i,
            ImageIds::height_marker_88_i,
            ImageIds::height_marker_89_i,
            ImageIds::height_marker_90_i,
            ImageIds::height_marker_91_i,
            ImageIds::height_marker_92_i,
            ImageIds::height_marker_93_i,
            ImageIds::height_marker_94_i,
            ImageIds::height_marker_95_i,
            ImageIds::height_marker_96_i,
            ImageIds::height_marker_97_i,
            ImageIds::height_marker_98_i,
            ImageIds::height_marker_99_i,
            ImageIds::height_marker_100_i,
            ImageIds::height_marker_101_i,
            ImageIds::height_marker_102_i,
            ImageIds::height_marker_103_i,
            ImageIds::height_marker_104_i,
            ImageIds::height_marker_105_i,
            ImageIds::height_marker_106_i,
            ImageIds::height_marker_107_i,
            ImageIds::height_marker_108_i,
            ImageIds::height_marker_109_i,
            ImageIds::height_marker_110_i,
            ImageIds::height_marker_111_i,
            ImageIds::height_marker_112_i,
            ImageIds::height_marker_113_i,
            ImageIds::height_marker_114_i,
            ImageIds::height_marker_115_i,
            ImageIds::height_marker_116_i,
            ImageIds::height_marker_117_i,
            ImageIds::height_marker_118_i,
            ImageIds::height_marker_119_i,
            ImageIds::height_marker_120_i,
            ImageIds::height_marker_121_i,
            ImageIds::height_marker_122_i,
            ImageIds::height_marker_123_i,
            ImageIds::height_marker_124_i,
            ImageIds::height_marker_125_i,
            ImageIds::height_marker_126_i,
            ImageIds::height_marker_127_i,
        },
        // Metric
        std::array<uint32_t, 256>{
            ImageIds::height_marker_neg_128_m,
            ImageIds::height_marker_neg_127_m,
            ImageIds::height_marker_neg_126_m,
            ImageIds::height_marker_neg_125_m,
            ImageIds::height_marker_neg_124_m,
            ImageIds::height_marker_neg_123_m,
            ImageIds::height_marker_neg_122_m,
            ImageIds::height_marker_neg_121_m,
            ImageIds::height_marker_neg_120_m,
            ImageIds::height_marker_neg_119_m,
            ImageIds::height_marker_neg_118_m,
            ImageIds::height_marker_neg_117_m,
            ImageIds::height_marker_neg_116_m,
            ImageIds::height_marker_neg_115_m,
            ImageIds::height_marker_neg_114_m,
            ImageIds::height_marker_neg_113_m,
            ImageIds::height_marker_neg_112_m,
            ImageIds::height_marker_neg_111_m,
            ImageIds::height_marker_neg_110_m,
            ImageIds::height_marker_neg_109_m,
            ImageIds::height_marker_neg_108_m,
            ImageIds::height_marker_neg_107_m,
            ImageIds::height_marker_neg_106_m,
            ImageIds::height_marker_neg_105_m,
            ImageIds::height_marker_neg_104_m,
            ImageIds::height_marker_neg_103_m,
            ImageIds::height_marker_neg_102_m,
            ImageIds::height_marker_neg_101_m,
            ImageIds::height_marker_neg_100_m,
            ImageIds::height_marker_neg_99_m,
            ImageIds::height_marker_neg_98_m,
            ImageIds::height_marker_neg_97_m,
            ImageIds::height_marker_neg_96_m,
            ImageIds::height_marker_neg_95_m,
            ImageIds::height_marker_neg_94_m,
            ImageIds::height_marker_neg_93_m,
            ImageIds::height_marker_neg_92_m,
            ImageIds::height_marker_neg_91_m,
            ImageIds::height_marker_neg_90_m,
            ImageIds::height_marker_neg_89_m,
            ImageIds::height_marker_neg_88_m,
            ImageIds::height_marker_neg_87_m,
            ImageIds::height_marker_neg_86_m,
            ImageIds::height_marker_neg_85_m,
            ImageIds::height_marker_neg_84_m,
            ImageIds::height_marker_neg_83_m,
            ImageIds::height_marker_neg_82_m,
            ImageIds::height_marker_neg_81_m,
            ImageIds::height_marker_neg_80_m,
            ImageIds::height_marker_neg_79_m,
            ImageIds::height_marker_neg_78_m,
            ImageIds::height_marker_neg_77_m,
            ImageIds::height_marker_neg_76_m,
            ImageIds::height_marker_neg_75_m,
            ImageIds::height_marker_neg_74_m,
            ImageIds::height_marker_neg_73_m,
            ImageIds::height_marker_neg_72_m,
            ImageIds::height_marker_neg_71_m,
            ImageIds::height_marker_neg_70_m,
            ImageIds::height_marker_neg_69_m,
            ImageIds::height_marker_neg_68_m,
            ImageIds::height_marker_neg_67_m,
            ImageIds::height_marker_neg_66_m,
            ImageIds::height_marker_neg_65_m,
            ImageIds::height_marker_neg_64_m,
            ImageIds::height_marker_neg_63_m,
            ImageIds::height_marker_neg_62_m,
            ImageIds::height_marker_neg_61_m,
            ImageIds::height_marker_neg_60_m,
            ImageIds::height_marker_neg_59_m,
            ImageIds::height_marker_neg_58_m,
            ImageIds::height_marker_neg_57_m,
            ImageIds::height_marker_neg_56_m,
            ImageIds::height_marker_neg_55_m,
            ImageIds::height_marker_neg_54_m,
            ImageIds::height_marker_neg_53_m,
            ImageIds::height_marker_neg_52_m,
            ImageIds::height_marker_neg_51_m,
            ImageIds::height_marker_neg_50_m,
            ImageIds::height_marker_neg_49_m,
            ImageIds::height_marker_neg_48_m,
            ImageIds::height_marker_neg_47_m,
            ImageIds::height_marker_neg_46_m,
            ImageIds::height_marker_neg_45_m,
            ImageIds::height_marker_neg_44_m,
            ImageIds::height_marker_neg_43_m,
            ImageIds::height_marker_neg_42_m,
            ImageIds::height_marker_neg_41_m,
            ImageIds::height_marker_neg_40_m,
            ImageIds::height_marker_neg_39_m,
            ImageIds::height_marker_neg_38_m,
            ImageIds::height_marker_neg_37_m,
            ImageIds::height_marker_neg_36_m,
            ImageIds::height_marker_neg_35_m,
            ImageIds::height_marker_neg_34_m,
            ImageIds::height_marker_neg_33_m,
            ImageIds::height_marker_neg_32_m,
            ImageIds::height_marker_neg_31_m,
            ImageIds::height_marker_neg_30_m,
            ImageIds::height_marker_neg_29_m,
            ImageIds::height_marker_neg_28_m,
            ImageIds::height_marker_neg_27_m,
            ImageIds::height_marker_neg_26_m,
            ImageIds::height_marker_neg_25_m,
            ImageIds::height_marker_neg_24_m,
            ImageIds::height_marker_neg_23_m,
            ImageIds::height_marker_neg_22_m,
            ImageIds::height_marker_neg_21_m,
            ImageIds::height_marker_neg_20_m,
            ImageIds::height_marker_neg_19_m,
            ImageIds::height_marker_neg_18_m,
            ImageIds::height_marker_neg_17_m,
            ImageIds::height_marker_neg_16_m,
            ImageIds::height_marker_neg_15_m,
            ImageIds::height_marker_neg_14_m,
            ImageIds::height_marker_neg_13_m,
            ImageIds::height_marker_neg_12_m,
            ImageIds::height_marker_neg_11_m,
            ImageIds::height_marker_neg_10_m,
            ImageIds::height_marker_neg_9_m,
            ImageIds::height_marker_neg_8_m,
            ImageIds::height_marker_neg_7_m,
            ImageIds::height_marker_neg_6_m,
            ImageIds::height_marker_neg_5_m,
            ImageIds::height_marker_neg_4_m,
            ImageIds::height_marker_neg_3_m,
            ImageIds::height_marker_neg_2_m,
            ImageIds::height_marker_neg_1_m,
            ImageIds::height_marker_0_m,
            ImageIds::height_marker_1_m,
            ImageIds::height_marker_2_m,
            ImageIds::height_marker_3_m,
            ImageIds::height_marker_4_m,
            ImageIds::height_marker_5_m,
            ImageIds::height_marker_6_m,
            ImageIds::height_marker_7_m,
            ImageIds::height_marker_8_m,
            ImageIds::height_marker_9_m,
            ImageIds::height_marker_10_m,
            ImageIds::height_marker_11_m,
            ImageIds::height_marker_12_m,
            ImageIds::height_marker_13_m,
            ImageIds::height_marker_14_m,
            ImageIds::height_marker_15_m,
            ImageIds::height_marker_16_m,
            ImageIds::height_marker_17_m,
            ImageIds::height_marker_18_m,
            ImageIds::height_marker_19_m,
            ImageIds::height_marker_20_m,
            ImageIds::height_marker_21_m,
            ImageIds::height_marker_22_m,
            ImageIds::height_marker_23_m,
            ImageIds::height_marker_24_m,
            ImageIds::height_marker_25_m,
            ImageIds::height_marker_26_m,
            ImageIds::height_marker_27_m,
            ImageIds::height_marker_28_m,
            ImageIds::height_marker_29_m,
            ImageIds::height_marker_30_m,
            ImageIds::height_marker_31_m,
            ImageIds::height_marker_32_m,
            ImageIds::height_marker_33_m,
            ImageIds::height_marker_34_m,
            ImageIds::height_marker_35_m,
            ImageIds::height_marker_36_m,
            ImageIds::height_marker_37_m,
            ImageIds::height_marker_38_m,
            ImageIds::height_marker_39_m,
            ImageIds::height_marker_40_m,
            ImageIds::height_marker_41_m,
            ImageIds::height_marker_42_m,
            ImageIds::height_marker_43_m,
            ImageIds::height_marker_44_m,
            ImageIds::height_marker_45_m,
            ImageIds::height_marker_46_m,
            ImageIds::height_marker_47_m,
            ImageIds::height_marker_48_m,
            ImageIds::height_marker_49_m,
            ImageIds::height_marker_50_m,
            ImageIds::height_marker_51_m,
            ImageIds::height_marker_52_m,
            ImageIds::height_marker_53_m,
            ImageIds::height_marker_54_m,
            ImageIds::height_marker_55_m,
            ImageIds::height_marker_56_m,
            ImageIds::height_marker_57_m,
            ImageIds::height_marker_58_m,
            ImageIds::height_marker_59_m,
            ImageIds::height_marker_60_m,
            ImageIds::height_marker_61_m,
            ImageIds::height_marker_62_m,
            ImageIds::height_marker_63_m,
            ImageIds::height_marker_64_m,
            ImageIds::height_marker_65_m,
            ImageIds::height_marker_66_m,
            ImageIds::height_marker_67_m,
            ImageIds::height_marker_68_m,
            ImageIds::height_marker_69_m,
            ImageIds::height_marker_70_m,
            ImageIds::height_marker_71_m,
            ImageIds::height_marker_72_m,
            ImageIds::height_marker_73_m,
            ImageIds::height_marker_74_m,
            ImageIds::height_marker_75_m,
            ImageIds::height_marker_76_m,
            ImageIds::height_marker_77_m,
            ImageIds::height_marker_78_m,
            ImageIds::height_marker_79_m,
            ImageIds::height_marker_80_m,
            ImageIds::height_marker_81_m,
            ImageIds::height_marker_82_m,
            ImageIds::height_marker_83_m,
            ImageIds::height_marker_84_m,
            ImageIds::height_marker_85_m,
            ImageIds::height_marker_86_m,
            ImageIds::height_marker_87_m,
            ImageIds::height_marker_88_m,
            ImageIds::height_marker_89_m,
            ImageIds::height_marker_90_m,
            ImageIds::height_marker_91_m,
            ImageIds::height_marker_92_m,
            ImageIds::height_marker_93_m,
            ImageIds::height_marker_94_m,
            ImageIds::height_marker_95_m,
            ImageIds::height_marker_96_m,
            ImageIds::height_marker_97_m,
            ImageIds::height_marker_98_m,
            ImageIds::height_marker_99_m,
            ImageIds::height_marker_100_m,
            ImageIds::height_marker_101_m,
            ImageIds::height_marker_102_m,
            ImageIds::height_marker_103_m,
            ImageIds::height_marker_104_m,
            ImageIds::height_marker_105_m,
            ImageIds::height_marker_106_m,
            ImageIds::height_marker_107_m,
            ImageIds::height_marker_108_m,
            ImageIds::height_marker_109_m,
            ImageIds::height_marker_110_m,
            ImageIds::height_marker_111_m,
            ImageIds::height_marker_112_m,
            ImageIds::height_marker_113_m,
            ImageIds::height_marker_114_m,
            ImageIds::height_marker_115_m,
            ImageIds::height_marker_116_m,
            ImageIds::height_marker_117_m,
            ImageIds::height_marker_118_m,
            ImageIds::height_marker_119_m,
            ImageIds::height_marker_120_m,
            ImageIds::height_marker_121_m,
            ImageIds::height_marker_122_m,
            ImageIds::height_marker_123_m,
            ImageIds::height_marker_124_m,
            ImageIds::height_marker_125_m,
            ImageIds::height_marker_126_m,
            ImageIds::height_marker_127_m,
        },
    };

    int8_t getTrackDecorationHeightOffset(const bool isFirstTile, const uint8_t trackId)
    {
        if (isFirstTile)
        {
            return _4F86B4[trackId * 2];
        }
        else
        {
            return _4F86B4[trackId * 2 + 1];
        }
    }

    uint32_t getHeightMarkerImage(const coord_t height)
    {
        const auto offset = height / Map::kMicroZStep - getGameState().kSeaLevel + 128;
        return kHeightMarkerImages[Config::get().heightMarkerOffset / 0x100][offset];
    }
}
