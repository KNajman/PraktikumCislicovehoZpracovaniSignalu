% =========================================================================
% AUDIO LOOPBACK TEST - Refactored 2024
% =========================================================================
% Tento skript generuje testovací signál, přehrává ho přes zvukovou kartu
% a současně nahrává odezvu pro analýzu (THD, Spektrum).
% =========================================================================

clear; clc; close all;

try
    % --- 1. OCHRANA PROTI VÍCENÁSOBNÉMU SPUŠTĚNÍ (Lab specific) ---
    if IsON_loop(1) ~= 1
        disp(["....it's still running ... please wait ..." + IsON_loop(-1)]);
        return; 
    end

    % Vyčištění prostředí
    clearvars -except IsON_loop; 
    clc;

    % =====================================================================
    % 2. KONFIGURACE (Zde měňte parametry)
    % =====================================================================
    CFG.Fs = 48000;           % Vzorkovací frekvence [Hz]
    CFG.Duration = 5;         % Délka testu [s]
    CFG.ThresholdStart = 0.95;% Práh pro detekci začátku signálu (0-1)
    CFG.BitDepth = 24;        % Bitová hloubka
    
    % SCÉNÁŘE: 'SINE_STEREO', 'CHIRP', 'MULTITONE', 'SILENCE'
    CFG.Scenario = 'SINE_STEREO'; 
    
    % Parametry pro generování signálů
    CFG.FreqL = 1000;         % Frekvence Levý kanál (pro SINE)
    CFG.FreqR = 1200;         % Frekvence Pravý kanál (pro SINE)
    CFG.Amp = 0.5;            % Amplituda

    % =====================================================================
    % 3. GENEROVÁNÍ SIGNÁLU
    % =====================================================================
    t = 0:1/CFG.Fs : CFG.Duration - 1/CFG.Fs; % Časová osa
    x_stereo = zeros(length(t), 2);           % Inicializace (L, R)

    switch CFG.Scenario
        case 'SINE_STEREO'
            % Klasický test: vlevo jedna frekvence, vpravo jiná
            x_stereo(:, 1) = CFG.Amp * cos(2*pi*CFG.FreqL*t);
            x_stereo(:, 2) = CFG.Amp * cos(2*pi*CFG.FreqR*t);
            
        case 'CHIRP'
            % Test frekvenční charakteristiky
            sig_chirp = chirp(t, 20, t(end), CFG.Fs/2-100);
            x_stereo(:, 1) = CFG.Amp * sig_chirp;
            x_stereo(:, 2) = CFG.Amp * sig_chirp; % Mono to Stereo
            
        case 'MULTITONE'
            % Příklad sčítání více frekvencí
            freqs = [1000, 5000, 10000, 15000];
            sig_multi = 0;
            for f = freqs
                sig_multi = sig_multi + (0.2 * cos(2*pi*f*t));
            end
            x_stereo(:, 1) = sig_multi;
            x_stereo(:, 2) = sig_multi;
            
        case 'SILENCE'
            % Ticho pro měření šumu pozadí
            x_stereo(:, :) = 0;
            
        otherwise
            error('Neznámý scénář testu.');
    end

    % Zobrazení spektra VSTUPNÍHO signálu (pro kontrolu)
    [ID_input, ID_output] = GetAudioID_4_USB_HQ; % Získání ID zařízení
    disp("Input ID: " + ID_input + " | Output ID: " + ID_output);
    
    % Inicializace grafů
    Pix_SS = get(0,'screensize'); 
    width = (Pix_SS(3)-300)/2; height = Pix_SS(4)/2;
    
    f_spect = figure('Position', [100 (Pix_SS(4)-height)/2 width height], ...
                     'Name', 'Porovnání spekter: Vstup (čárkovaně) vs Výstup (plně)');
    figure(f_spect); hold on;
    
    % Vykreslení ideálního vstupu do grafu (referenční čárkovaná čára)
    % Pozn: Předpokládám, že AmplSpectr vrací data pro plot
    AmplSpectr(x_stereo(:,1), length(x_stereo), CFG.Fs, 'LineStyle','-.', 'Color', 'blue');
    AmplSpectr(x_stereo(:,2), length(x_stereo), CFG.Fs, 'LineStyle','-.', 'Color', 'red');
    subtitle('Modrá: Levý, Červená: Pravý. Čárkovaně = Generovaný signál.');

    % =====================================================================
    % 4. PŘEHRÁVÁNÍ A NAHRÁVÁNÍ (Loopback)
    % =====================================================================
    playerObj = audioplayer(x_stereo, CFG.Fs, CFG.BitDepth, ID_output);
    recObj    = audiorecorder(CFG.Fs, CFG.BitDepth, 2, ID_input);
    
    disp("    ...Start Playback & Record...");
    tic;
    record(recObj);         % Start nahrávání (na pozadí)
    play(playerObj);        % Start přehrávání
    pause(CFG.Duration + 0.5); % Čekání na dokončení (+ rezerva)
    stop(recObj);
    toc;

    % Získání surových dat
    y_raw = getaudiodata(recObj);

    % =====================================================================
    % 5. ZPRACOVÁNÍ A OŘEZÁNÍ SIGNÁLU
    % =====================================================================
    % Detekce začátku signálu na základě prahu
    % Hledáme první vzorek, který překročí X % maxima
    thresh_val_L = max(abs(y_raw(:,1))) * CFG.ThresholdStart;
    thresh_val_R = max(abs(y_raw(:,2))) * CFG.ThresholdStart;
    
    idx_start_L = find(y_raw(:,1) > thresh_val_L, 1);
    idx_start_R = find(y_raw(:,2) > thresh_val_R, 1);
    
    % Pokud se nic nenašlo (např. ticho), začneme od 1
    if isempty(idx_start_L), idx_start_L = 1; end
    if isempty(idx_start_R), idx_start_R = 1; end
    
    % Použijeme pozdější start + malý posun pro stabilitu
    idx_start = max([idx_start_L, idx_start_R]) + 20;
    
    if idx_start >= length(y_raw)
        warning('Signál nebyl detekován nebo je příliš krátký.');
        y_trimmed = y_raw; % Fallback
    else
        y_trimmed = y_raw(idx_start:end, :);
    end
    
    y1 = y_trimmed(:, 1); % Změřený Levý
    y2 = y_trimmed(:, 2); % Změřený Pravý

    % =====================================================================
    % 6. VYKRESLENÍ A VÝPOČTY (THD, Spektrum)
    % =====================================================================
    
    % A) Spektrum naměřeného signálu (do stejného grafu jako vstup)
    figure(f_spect); 
    AX_val(1) = max(AmplSpectr(y1, length(y1), CFG.Fs, 'Color','blue')); 
    AX_val(2) = max(AmplSpectr(y2, length(y2), CFG.Fs, 'Color','red'));
    ylim([-0.05*max(AX_val) 1.1*max(AX_val)]);
    
    % B) Časový průběh a THD
    f_signal = figure('Position', [(Pix_SS(3)-width-100) (Pix_SS(4)-height)/2 width height], ...
                      'Name','Zaznamenaný signál v čase');
    
    THD_L = thd(y1, CFG.Fs, 'aliased'); 
    THD_R = thd(y2, CFG.Fs, 'aliased');
    
    % Levý kanál
    ax1 = subplot(2,1,1); 
    plot(y1, 'Color', 'blue'); 
    title("Levý kanál | THD = " + THD_L + " [dBc]");
    grid on; ylim([1.2*min(y1) 1.2*max(y1)]);
    
    % Pravý kanál
    ax2 = subplot(2,1,2); 
    plot(y2, 'Color', 'red'); 
    title("Pravý kanál | THD = " + THD_R + " [dBc]");
    grid on; ylim([1.2*min(y2) 1.2*max(y2)]);
    
    linkaxes([ax1, ax2], 'x');

    % Výpis do konzole
    disp("------------------------------------------------");
    disp("Výsledky měření:");
    disp("Start prahu: " + CFG.ThresholdStart + " | Fs: " + CFG.Fs + " Hz");
    disp("THD Levý: " + THD_L + " dBc");
    disp("THD Pravý: " + THD_R + " dBc");
    disp("Délka vzorku: " + length(y1) + " samples (" + length(y1)/CFG.Fs + " s)");
    
    % Korektní ukončení smyčky
    disp(["LOOP done with exit code: " + IsON_loop(-1)]);

catch ME
    % Error handling
    disp(["LOOP done with errors!!! Exit code: " + IsON_loop(-1)]);
    rethrow(ME);
end