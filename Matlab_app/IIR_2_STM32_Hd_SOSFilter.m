% Je rozdil mezi SOSFilter a df2sos strukturou
function sr = IIR_2_STM32_Hd_SOSFilter(Hd)
    A=Hd.Denominator;
    B=Hd.Numerator;
    G=Hd.ScaleValues;
    sr =0;
    nS=length(G) - 1;
    n5=1;
    for n=1:nS(1)
       sr(n5:n5+2) = B(n, 1:3).*G(n);
       sr(n5+3:n5+4) = A(n, 2:3);
       n5=n5+5;
    end
end