function My_Extern_Button_CV09(app)
    % f=load('IIR_PP_CV09.mat');
    % sr=IIR_2_STM32(f.SOS, f.G);% left channel
    
    Fstop1 = 100;   % First Stopband Frequency
    Fpass1 = 1000;   % First Passband Frequency
    Fpass2 = 5000;   % Second Passband Frequency
    Fstop2 = 10000;   % Second Stopband Frequency
    Hd=My_IIR_PP_Hd(Fstop1, Fpass1,Fpass2,Fstop2);
    
    sr=IIR_2_STM32_Hd(Hd);% left channel
    writeDataSTM32(app.s, 35009, length(sr), sr);
    [h, w]=freqz(Hd, 10000, app.Fs);
    figure; plot(w,abs(h));
    figure; plot(w,20*log10(abs(h)));
end