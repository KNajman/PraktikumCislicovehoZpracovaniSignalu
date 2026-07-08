%2023-04-05 (USB Advanced Audio Device)
%2023-10-02 test for new interface
close all
clear *
clc
%Fs=2*48000; % sampling frequency
Fs=48000; % sampling frequency
N=48000; % Number of samples to be processed
% GetAudioID; % (USB Advanced Audio Device)
% audio balance on PC for directly connection from output to input (wire): 
[ID_input, ID_output] = GetAudioID_4_USB_HQ
% audio balance for connection PC_2_PMOD_2_PC:
% G=0.95; % gain 
% input 50, output 50, f=1000Hz,  xL = G.*cos(2*pi*1000*t+pi); xR = G.*cos(2*pi*1000*t);


% Fs=8000;
% N=8000;
% Fs=16000;
% N=16000;
%Fs=96000;
%N=96000;
tStart=datetime('now'); disp("Start...");
fc=1200;
T=5; %s .... time of test 

t=0:1/Fs:T-1/Fs;
x(1:T.*Fs)=0;
% % a=(Fs/2000);
% % a=1;
% % for f=100:200:Fs/2 - 100
% %     x= x + cos(2*pi*f*t+pi/a);
% %     a=a+0.01;
% % end

% %x=x ./100;
% x=x ./250;

%x = 0.3*cos(2*pi*1000*t)+0.2*cos(2*pi*10000*t);
x2=x;

G=0.95; % gain 
x = G.*cos(2*pi*2000*t);
% x = 0.45*(cos(2*pi*5000*t+pi)+ cos(2*pi*17320*t+pi));
x2 = G.*cos(2*pi*12000*t);

%add GAP
%x(1.83*Fs:1.83*Fs+100)=0;
%x(1.85*Fs:1.85*Fs+100)=0;
%
% AmplSpectr(x+x2, N, Fs); % wrong idea
hold off;
AmplSpectr(x, N, Fs);
hold on; 
AmplSpectr(x2, N, Fs); 
hold off;
x12(:, 1)=x; %stereo signal
x12(:, 2)=x2;

playerObj = audioplayer(x12,Fs, 24, ID_output);disp("    ...playing...");
start = 1;
play(playerObj,start);
pause(1); % wait some time for signal stabilization, 
recObj = audiorecorder(Fs,24,2,ID_input); disp("             ...recording...");
%recordblocking(recObj, 4);% record 4 seconds
recordblocking(recObj, 2);% record 2 seconds it is sufficient for basic experiments

y12o = getaudiodata(recObj);
%y12=y12o(Fs:3*Fs, :);%take 2s of signal from 1s to end....
y12=y12o(Fs:2*Fs, :);%take 1s of signal from 1s to end....
% y12=y12o;

y1=y12(:, 1);% Left channel
y2=y12(:, 2);% Right chan.
figure
AmplSpectr(y1, N, Fs);
hold on
AmplSpectr(y2, N, Fs);
figure

%plot(x(1:1000));
hold on
%plot(y1(1:4000)); plot(y2(1:4000));
%plot(y1(1:400)); plot(y2(1:400));
plot(y1(1:1800)); plot(y2(1:1800));

disp(["duration of the experiment: " + string(minus (datetime('now'), tStart ))]);