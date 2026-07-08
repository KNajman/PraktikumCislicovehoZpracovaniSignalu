function sr = IIR_2_STM32_Hd(Hd)
    SOS=Hd.sosMatrix;
    G=Hd.ScaleValues;
    sr =0;
    nS=size(SOS);
    n5=1;
    for n=1:nS(1)
       sr(n5:n5+2) = SOS(n, 1:3).*G(n);
       sr(n5+3:n5+4) = -SOS(n, 5:6);
       n5=n5+5;
    end
end