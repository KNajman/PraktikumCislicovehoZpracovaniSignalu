function My_Extern_Button_CV08(app)
    % Fstop1 = 2200;   % First Stopband Frequency
% Fpass1 = 3600;   % First Passband Frequency
% Fpass2 = 5000;   % Second Passband Frequency
% Fstop2 = 5800;   % Second Stopband Frequency
    [b, nb]=My_FIR(1000,2000,50);
    writeDataSTM32(app.s, 35008, nb, b);
    [h, w]=freqz(b,1, 1000, app.Fs);
    figure(1); plot(w,abs(h));
    figure(2); plot(w,20*log10(abs(h)));
end