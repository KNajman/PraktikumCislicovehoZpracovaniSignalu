%Function plots the one-sided amplitude spectrum of the signal x
%N is the number of samples
%Fs is the sampling frequency
%%  Funkce vykresli jednostranne amplitudove spektrum signalu x
%  N je pocet vzorku
%  Fs je vzorkovaci frekvence
%  2023-10-26 upraven radek :  NX=(0:Fs/N:Fs/2-Fs/N);
%  pridan popis os
%%
function [AX, NX] = AmplSpectr(x, N,Fs,varargin)
    N=fix(N);
    X=fft(x, N)./N;%normovane spektrum
    AX = 2*abs(X(1:fix(N/2)));
    AX(1)=AX(1)/2; %ss slozka
    NX=(0:Fs/N:Fs/2-Fs/N);
    stem(NX,AX,varargin{:});
    title('Jednostranne amlitudove spektrum');
    xlabel('frekvence [Hz]');
    ylabel('Amplituda');
    ylim([-0.05*max(AX) 1.1 .* max(AX)] );
end