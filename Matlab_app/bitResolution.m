function [nBits] = bitResolution(inputSignal)
%BITRESOLUTION Summary of this function goes here
%   Detailed explanation goes here
    y1=sort(inputSignal);
    mi=min(y1);  ma=max(y1);
    y=(y1-mi)/(ma-mi);
    d=diff(y);
    % d=d(find(d));
    % d1=find(d);
    d2=d(d~=0);
    a=max(1/min(d2));
    nBits=ceil(log10(a)/log10(2));
end

