    float luma_filter[33] = {
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000
    };
    
    float chroma_filter[33] = {
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000,
    0.000000000
    };
    
    if (VHS_BleedMode == 0 || VHS_BleedMode == 3){ //Three Phase and Three Phase (RetroArch)
        luma_filter[0] =    -0.000012020;
        luma_filter[1] =    -0.000022146;
        luma_filter[2] =    -0.000013155;
        luma_filter[3] =    -0.000012020;
        luma_filter[4] =    -0.000049979;
        luma_filter[5] =    -0.000113940;
        luma_filter[6] =    -0.000122150;
        luma_filter[7] =    -0.000005612;
        luma_filter[8] =    0.000170516;
        luma_filter[9] =    0.000237199;
        luma_filter[10] =    0.000169640;
        luma_filter[11] =    0.000285688;
        luma_filter[12] =    0.000984574;
        luma_filter[13] =    0.002018683;
        luma_filter[14] =    0.002002275;
        luma_filter[15] =    -0.000909882;
        luma_filter[16] =    -0.007049081;
        luma_filter[17] =    -0.013222860;
        luma_filter[18] =    -0.012606931;
        luma_filter[19] =    0.002460860;
        luma_filter[20] =    0.035868225;
        luma_filter[21] =    0.084016453;
        luma_filter[22] =    0.135563500;
        luma_filter[23] =    0.175261268;
        luma_filter[24] =    0.190176552;

        chroma_filter[0] =    -0.000118847;
        chroma_filter[1] =    -0.000271306;
        chroma_filter[2] =    -0.000502642;
        chroma_filter[3] =    -0.000930833;
        chroma_filter[4] =    -0.001451013;
        chroma_filter[5] =    -0.002064744;
        chroma_filter[6] =    -0.002700432;
        chroma_filter[7] =    -0.003241276;
        chroma_filter[8] =    -0.003524948;
        chroma_filter[9] =    -0.003350284;
        chroma_filter[10] =    -0.002491729;
        chroma_filter[11] =    -0.000721149;
        chroma_filter[12] =    0.002164659;
        chroma_filter[13] =    0.006313635;
        chroma_filter[14] =    0.011789103;
        chroma_filter[15] =    0.018545660;
        chroma_filter[16] =    0.026414396;
        chroma_filter[17] =    0.035100710;
        chroma_filter[18] =    0.044196567;
        chroma_filter[19] =    0.053207202;
        chroma_filter[20] =    0.061590275;
        chroma_filter[21] =    0.068803602;
        chroma_filter[22] =    0.074356193;
        chroma_filter[23] =    0.077856564;
        chroma_filter[24] =    0.079052396;
    }
    
    else if (VHS_BleedMode == 1) { //Old Three-Phase
        luma_filter[0] = -0.000071070;
        luma_filter[1] = -0.000032816;
        luma_filter[2] = 0.000128784;
        luma_filter[3] = 0.000134711;
        luma_filter[4] = -0.000226705;
        luma_filter[5] = -0.000777988;
        luma_filter[6] = -0.000997809;
        luma_filter[7] = -0.000522802;
        luma_filter[8] = 0.000344691;
        luma_filter[9] = 0.000768930;
        luma_filter[10] = 0.000275591;
        luma_filter[11] = -0.000373434;
        luma_filter[12] = 0.000522796;
        luma_filter[13] = 0.003813817;
        luma_filter[14] = 0.007502825;
        luma_filter[15] = 0.006786001;
        luma_filter[16] = -0.002636726;
        luma_filter[17] = -0.019461182;
        luma_filter[18] = -0.033792479;
        luma_filter[19] = -0.029921972;
        luma_filter[20] = 0.005032552;
        luma_filter[21] = 0.071226466;
        luma_filter[22] = 0.151755921;
        luma_filter[23] = 0.218166470;
        luma_filter[24] = 0.243902439;
            
        chroma_filter[0] = 0.001845562;
        chroma_filter[1] = 0.002381606;
        chroma_filter[2] = 0.003040177;
        chroma_filter[3] = 0.003838976;
        chroma_filter[4] = 0.004795341;
        chroma_filter[5] = 0.005925312;
        chroma_filter[6] = 0.007242534;
        chroma_filter[7] = 0.008757043;
        chroma_filter[8] = 0.010473987;
        chroma_filter[9] = 0.012392365;
        chroma_filter[10] = 0.014503872;
        chroma_filter[11] = 0.016791957;
        chroma_filter[12] = 0.019231195;
        chroma_filter[13] = 0.021787070;
        chroma_filter[14] = 0.024416251;
        chroma_filter[15] = 0.027067414;
        chroma_filter[16] = 0.029682613;
        chroma_filter[17] = 0.032199202;
        chroma_filter[18] = 0.034552198;
        chroma_filter[19] = 0.036677005;
        chroma_filter[20] = 0.038512317;
        chroma_filter[21] = 0.040003044;
        chroma_filter[22] = 0.041103048;
        chroma_filter[23] = 0.041777517;
        chroma_filter[24] = 0.042004791;
    }
    
    else if (VHS_BleedMode == 2) { //Two-Phase
        luma_filter[0] =    -0.000205844;
        luma_filter[1] =    -0.000149453;
        luma_filter[2] =    -0.000051693;
        luma_filter[3] =    0.000000000;
        luma_filter[4] =    -0.000066171;
        luma_filter[5] =    -0.000245058;
        luma_filter[6] =    -0.000432928;
        luma_filter[7] =    -0.000472644;
        luma_filter[8] =    -0.000252236;
        luma_filter[9] =    0.000198929;
        luma_filter[10] =    0.000687058;
        luma_filter[11] =    0.000944112;
        luma_filter[12] =    0.000803467;
        luma_filter[13] =    0.000363199;
        luma_filter[14] =    0.000013422;
        luma_filter[15] =    0.000253402;
        luma_filter[16] =    0.001339461;
        luma_filter[17] =    0.002932972;
        luma_filter[18] =    0.003983485;
        luma_filter[19] =    0.003026683;
        luma_filter[20] =    -0.001102056;
        luma_filter[21] =    -0.008373026;
        luma_filter[22] =    -0.016897700;
        luma_filter[23] =    -0.022914480;
        luma_filter[24] =    -0.021642347;
        luma_filter[25] =    -0.008863273;
        luma_filter[26] =    0.017271957;
        luma_filter[27] =    0.054921920;
        luma_filter[28] =    0.098342579;
        luma_filter[29] =    0.139044281;
        luma_filter[30] =    0.168055832;
        luma_filter[31] =    0.178571429;
        
        chroma_filter[0] =    0.001384762;
        chroma_filter[1] =    0.001678312;
        chroma_filter[2] =    0.002021715;
        chroma_filter[3] =    0.002420562;
        chroma_filter[4] =    0.002880460;
        chroma_filter[5] =    0.003406879;
        chroma_filter[6] =    0.004004985;
        chroma_filter[7] =    0.004679445; 
        chroma_filter[8] =    0.005434218;
        chroma_filter[9] =    0.006272332; 
        chroma_filter[10] =    0.007195654; 
        chroma_filter[11] =    0.008204665; 
        chroma_filter[12] =    0.009298238; 
        chroma_filter[13] =    0.010473450;
        chroma_filter[14] =    0.011725413; 
        chroma_filter[15] =    0.013047155;
        chroma_filter[16] =    0.014429548;
        chroma_filter[17] =    0.015861306; 
        chroma_filter[18] =    0.017329037; 
        chroma_filter[19] =    0.018817382;
        chroma_filter[20] =    0.020309220; 
        chroma_filter[21] =    0.021785952;
        chroma_filter[22] =    0.023227857;
        chroma_filter[23] =    0.024614500; 
        chroma_filter[24] =    0.025925203;
        chroma_filter[25] =    0.027139546; 
        chroma_filter[26] =    0.028237893;
        chroma_filter[27] =    0.029201910; 
        chroma_filter[28] =    0.030015081; 
        chroma_filter[29] =    0.030663170;
        chroma_filter[30] =    0.031134640;
        chroma_filter[31] =    0.031420995; 
        chroma_filter[32] =    0.031517031;
    }
    
    else if (VHS_BleedMode == 4) { //Two-Phase (RetroArch)
        luma_filter[0] =  -0.000174844;
        luma_filter[1] =  -0.000205844;
        luma_filter[2] =  -0.000149453;
        luma_filter[3] =  -0.000051693;
        luma_filter[4] = 0.000000000;
        luma_filter[5] =  -0.000066171;
        luma_filter[6] =  -0.000245058;
        luma_filter[7] =  -0.000432928;
        luma_filter[8] =  -0.000472644;
        luma_filter[9] =  -0.000252236;
        luma_filter[10] =  0.000198929;
        luma_filter[11] =  0.000687058;
        luma_filter[12] =  0.000944112;
        luma_filter[13] =  0.000803467;
        luma_filter[14] =  0.000363199;
        luma_filter[15] =  0.000013422;
        luma_filter[16] =  0.000253402;
        luma_filter[17] =  0.001339461;
        luma_filter[18] =  0.002932972;
        luma_filter[19] =  0.003983485;
        luma_filter[20] =  0.003026683;
        luma_filter[21] =  -0.001102056;
        luma_filter[22] =  -0.008373026;
        luma_filter[23] =  -0.016897700;
        luma_filter[24] =  -0.022914480;
        luma_filter[25] =  -0.021642347;
        luma_filter[26] =  -0.008863273;
        luma_filter[27] =  0.017271957;
        luma_filter[28] =  0.054921920;
        luma_filter[29] =  0.098342579;
        luma_filter[30] =  0.139044281;
        luma_filter[31] =  0.168055832;
        luma_filter[32] =  0.178571429;
            
        chroma_filter[0] =   0.001384762;
        chroma_filter[1] =   0.001678312;
        chroma_filter[2] =   0.002021715;
        chroma_filter[3] =   0.002420562;
        chroma_filter[4] =   0.002880460;
        chroma_filter[5] =   0.003406879;
        chroma_filter[6] =   0.004004985;
        chroma_filter[7] =   0.004679445;
        chroma_filter[8] =   0.005434218;
        chroma_filter[9] =   0.006272332;
        chroma_filter[10] =   0.007195654;
        chroma_filter[11] =   0.008204665;
        chroma_filter[12] =   0.009298238;
        chroma_filter[13] =   0.010473450;
        chroma_filter[14] =   0.011725413;
        chroma_filter[15] =   0.013047155;
        chroma_filter[16] =   0.014429548;
        chroma_filter[17] =   0.015861306;
        chroma_filter[18] =   0.017329037;
        chroma_filter[19] =   0.018817382;
        chroma_filter[20] =   0.020309220;
        chroma_filter[21] =   0.021785952;
        chroma_filter[22] =   0.023227857;
        chroma_filter[23] =   0.024614500;
        chroma_filter[24] =   0.025925203;
        chroma_filter[25] =   0.027139546;
        chroma_filter[26] =   0.028237893;
        chroma_filter[27] =   0.029201910;
        chroma_filter[28] =   0.030015081;
        chroma_filter[29] =   0.030663170;
        chroma_filter[30] =   0.031134640;
        chroma_filter[31] =   0.031420995;
        chroma_filter[32] =   0.031517031;
    }