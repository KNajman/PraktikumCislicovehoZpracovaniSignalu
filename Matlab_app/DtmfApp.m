classdef DtmfApp < matlab.apps.AppBase

    % --- VLASTNOSTI (PROPERTIES) ---
    properties (Access = public)
        UIFigure        matlab.ui.Figure
        GridLayout      matlab.ui.container.GridLayout
        
        % Ovládací prvky
        DisplayArea     matlab.ui.control.EditField
        StatusLabel     matlab.ui.control.Label
        
        % UART Ovládání
        ComPortEdit     matlab.ui.control.EditField
        ConnectButton   matlab.ui.control.Button
        
        % Audio Analýza
        AudioResultLabel matlab.ui.control.Label 
        
        % Konzole a její ovládání
        LogArea         matlab.ui.control.TextArea 
        ClearLogButton  matlab.ui.control.Button % NOVÉ
    end

    properties (Access = private)
        % Audio nastavení
        Fs = 8000;           
        ToneLen = 0.256;       
        ListenLen = 0.4;      
        
        % UART nastavení
        SerialObj             
        BaudRate = 2*115200;    
    end

    % --- METODY (LOGIKA) ---
    methods (Access = private)

        % 1. PŘIPOJENÍ / ODPOJENÍ UARTU
        function toggleSerial(app)
            if ~isempty(app.SerialObj) && isvalid(app.SerialObj)
                try
                    delete(app.SerialObj);
                    app.SerialObj = [];
                    app.ConnectButton.Text = 'Připojit';
                    app.ConnectButton.BackgroundColor = [0.96 0.96 0.96];
                    app.logMessage('UART: Odpojeno.');
                catch e
                    app.logMessage(['Chyba při odpojování: ' e.message]);
                end
            else
                portName = app.ComPortEdit.Value;
                try
                    app.SerialObj = serialport(portName, app.BaudRate);
                    configureTerminator(app.SerialObj, "CR/LF");
                    configureCallback(app.SerialObj, "terminator", @(src, event) readSerialData(app, src));
                    
                    app.ConnectButton.Text = 'Odpojit';
                    app.ConnectButton.BackgroundColor = [0.6 1 0.6];
                    app.logMessage(['UART: Připojeno k ' portName]);
                catch e
                    app.StatusLabel.Text = 'Chyba připojení!';
                    app.StatusLabel.FontColor = [1 0 0];
                    app.logMessage(['CHYBA PORTU: ' e.message]);
                end
            end
        end

        % 2. ČTENÍ Z UARTU
        function readSerialData(app, src)
            try
                line = readline(src);
                line = strtrim(line);
                if ~isempty(line)
                    app.logMessage(['Nucleo UART: ' line]);
                    
                    if contains(line, "Detekovano:")
                        parts = split(line, ":");
                        if length(parts) > 1
                            % Zde lze zpracovat přijatý znak
                        end
                    end
                end
            catch
            end
        end

        % 3. HLAVNÍ AKCE
        function sendDtmf(app, charToSend)

            app.DisplayArea.Value = [app.DisplayArea.Value, charToSend];
            app.StatusLabel.Text = ['Odesílám: ' charToSend];
            app.StatusLabel.FontColor = 'black';
            drawnow;
            
            [sig, ~] = app.generate_tone(charToSend);
            sound(sig, app.Fs);
            app.logMessage(['Audio OUT: ' charToSend]);
            
            pause(0.05); 
            app.analyzeMicrophone();
        end

        % 4. ANALÝZA MIKROFONU
        function analyzeMicrophone(app)
            try
                app.StatusLabel.Text = 'Poslouchám mikrofon...';
                
                recObj = audiorecorder(app.Fs, 16, 1);
                %cvilková nahrávka pro vyčistění bufferu
                % recordblocking(recObj, 0.1);
                % getaudiodata(recObj);
                recordblocking(recObj, app.ListenLen);
                audioData = getaudiodata(recObj);
                
                [detChar, fL, fH] = app.decode_tone_fft(audioData);
                
                if ~isempty(detChar)
                    msg = sprintf('Mikrofon: %d Hz + %d Hz -> [%s]', round(fL), round(fH), detChar);
                    app.AudioResultLabel.Text = msg;
                    app.AudioResultLabel.FontColor = [0 0.5 0]; 
                    app.logMessage(msg);
                else
                    app.AudioResultLabel.Text = 'Mikrofon: Ticho nebo nedetekováno';
                    app.AudioResultLabel.FontColor = [0.5 0.5 0.5];
                end
                
                app.StatusLabel.Text = 'Připraveno.';
            catch e
                app.logMessage(['Chyba mikrofonu: ' e.message]);
            end
        end

        % 5. GENERÁTOR
        function [signal, t] = generate_tone(app, key)
            f_low = [697, 770, 852, 941];
            f_high = [1209, 1336, 1477, 1633];
            keyMap = ['1','2','3','A'; '4','5','6','B'; '7','8','9','C'; '*','0','#','D'];
            
            [r, c] = find(keyMap == key);
            t = 0:1/app.Fs:app.ToneLen;
            if ~isempty(r)
                signal = 0.8 * (sin(2*pi*f_low(r)*t) + sin(2*pi*f_high(c)*t));
            else
                signal = zeros(size(t));
            end
        end

        % 6. DEKODÉR
        function [charDetected, found_fL, found_fH] = decode_tone_fft(app, signal)
            charDetected = ''; found_fL = 0; found_fH = 0;
            if max(abs(signal)) < 0.02, return; end 

            N = length(signal);
            Y = abs(fft(signal));
            f_axis = (0:N-1)*(app.Fs/N);
            limit = floor(N/2);
            Y = Y(1:limit);
            f_axis = f_axis(1:limit);
            
            [~, locs] = findpeaks(Y, 'MinPeakHeight', max(Y)*0.2, 'MinPeakDistance', 50/(app.Fs/N));
            peak_freqs = f_axis(locs);

            DTMF_L = [697, 770, 852, 941];
            DTMF_H = [1209, 1336, 1477, 1633];
            idxL = []; idxH = [];
            tolerance = 30;
            
            for f = peak_freqs
                [minVal, i] = min(abs(DTMF_L - f));
                if minVal < tolerance, idxL = i; found_fL = f; end
                [minVal, i] = min(abs(DTMF_H - f));
                if minVal < tolerance, idxH = i; found_fH = f; end
            end
            
            keyMap = ['1','2','3','A'; '4','5','6','B'; '7','8','9','C'; '*','0','#','D'];
            if ~isempty(idxL) && ~isempty(idxH)
                charDetected = keyMap(idxL, idxH);
            end
        end

        % Logování
        function logMessage(app, msg)
            %ts = datestr(now, 'HH:MM:SS');
            ts = datetime("now","Format","HH:mm:ss");
            app.LogArea.Value = [sprintf('[%s] %s', ts, msg); app.LogArea.Value];
        end
        
        % NOVÉ: Funkce pro vymazání logu
        function clearLog(app)
            app.LogArea.Value = '';
        end
    end

    % --- TVORBA GUI ---
    methods (Access = public)
        function createComponents(app)
            % Okno (mírně zvětšeno na výšku)
            app.UIFigure = uifigure('Name', 'DTMF Master', 'Position', [100, 100, 400, 680]);
            
            % Grid Layout - ZMĚNA: 11 řádků místo 10
            g = uigridlayout(app.UIFigure, [11, 4]);
            % ZMĚNA: Přidán poslední rozměr '30px' pro tlačítko mazání
            g.RowHeight = {'1x', '1x', '0.5x', '1x', '1x', '1x', '1x', '0.5x', '0.5x', '3x', '1x'};
            app.GridLayout = g;

            % 1. ŘÁDEK: PŘIPOJENÍ
            lblPort = uilabel(g);
            lblPort.Text = 'Port:';
            lblPort.HorizontalAlignment = 'right';
            lblPort.Layout.Row = 1; lblPort.Layout.Column = 1;
            
            app.ComPortEdit = uieditfield(g, 'text');
            app.ComPortEdit.Value = 'COM6'; 
            app.ComPortEdit.Layout.Row = 1; app.ComPortEdit.Layout.Column = 2;
            
            app.ConnectButton = uibutton(g, 'push');
            app.ConnectButton.Text = 'Připojit';
            app.ConnectButton.Layout.Row = 1; app.ConnectButton.Layout.Column = [3 4];
            app.ConnectButton.ButtonPushedFcn = @(btn,event) toggleSerial(app);

            % 2. ŘÁDEK: DISPLEJ
            app.DisplayArea = uieditfield(g, 'text');
            app.DisplayArea.Layout.Row = 2; app.DisplayArea.Layout.Column = [1 4];
            app.DisplayArea.FontSize = 24;
            app.DisplayArea.HorizontalAlignment = 'right';
            app.DisplayArea.Editable = 'off';

            % 3. ŘÁDEK: STATUS
            app.StatusLabel = uilabel(g);
            app.StatusLabel.Layout.Row = 3; app.StatusLabel.Layout.Column = [1 4];
            app.StatusLabel.Text = 'Nepřipojeno.';
            app.StatusLabel.HorizontalAlignment = 'center';

            % 4.-7. ŘÁDEK: KLÁVESNICE
            labels = {'1','2','3','A'; '4','5','6','B'; '7','8','9','C'; '*','0','#','D'};
            for r = 1:4
                for c = 1:4
                    btn = uibutton(g, 'push');
                    btn.Layout.Row = r + 3;
                    btn.Layout.Column = c;
                    btn.Text = labels{r,c};
                    btn.FontSize = 18;
                    btn.FontWeight = 'bold';
                    btn.ButtonPushedFcn = @(btn,event) sendDtmf(app, btn.Text);
                end
            end
            
            % 8. ŘÁDEK: AUDIO LABEL
            lblAudio = uilabel(g);
            lblAudio.Text = 'Audio analýza:';
            lblAudio.FontWeight = 'bold';
            lblAudio.Layout.Row = 8; lblAudio.Layout.Column = [1 4];
            
            % 9. ŘÁDEK: AUDIO VÝSLEDEK
            app.AudioResultLabel = uilabel(g);
            app.AudioResultLabel.Text = '---';
            app.AudioResultLabel.Layout.Row = 9; app.AudioResultLabel.Layout.Column = [1 4];
            app.AudioResultLabel.HorizontalAlignment = 'center';
            app.AudioResultLabel.BackgroundColor = [0.9 0.9 0.9];

            % 10. ŘÁDEK: LOG (KONZOLE)
            app.LogArea = uitextarea(g);
            app.LogArea.Layout.Row = 10; app.LogArea.Layout.Column = [1 4];
            app.LogArea.Editable = 'off';
            app.LogArea.BackgroundColor = [0 0 0];
            app.LogArea.FontColor = [0 1 0];
            app.LogArea.FontName = 'Monospaced';
            
            % 11. ŘÁDEK: TLAČÍTKO VYMAZAT (NOVÉ)
            app.ClearLogButton = uibutton(g, 'push');
            app.ClearLogButton.Layout.Row = 11;
            app.ClearLogButton.Layout.Column = [1 4];
            app.ClearLogButton.Text = 'Vymazat konzoli';
            app.ClearLogButton.FontSize = 10;
            % Jednoduché volání funkce pro vymazání
            app.ClearLogButton.ButtonPushedFcn = @(btn,event) clearLog(app);
        end

        % Konstruktor
        function app = DtmfApp
            createComponents(app);
            app.UIFigure.Visible = 'on';
        end
        
        % Destruktor
        function delete(app)
            if ~isempty(app.SerialObj) && isvalid(app.SerialObj)
                delete(app.SerialObj);
            end
            delete(app.UIFigure);
        end
    end
end