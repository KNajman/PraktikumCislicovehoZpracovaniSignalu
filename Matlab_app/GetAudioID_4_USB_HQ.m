% Function returns the ID for the sound card "USB HQ AUDIO Mini adapter"
% Usage: 
% [ID_input, ID_output] = GetAudioID_4_USB_HQ
% If the adapter is not detected, it returns -1.
function [ID_input, ID_output] = GetAudioID_4_USB_HQ 
    audiodevreset;
    info = audiodevinfo;
    N=length(info.input);
    disp("INPUTS...");
    ID_input= -1; ID_output = -1;
    for n=1:N
        disp(info.input(n));
        if info.input(n).Name == "Mikrofon (USB Advanced Audio Device) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "Mikrofon (2 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "Mikrofon (3 - USB Advanced Audio Device) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "Mikrofon (3 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "Mikrofon (4 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "Mikrofon (5 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "Mikrofon (6 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
        if info.input(n).Name == "1-2 (OCTA-CAPTURE) (Windows DirectSound)"
            ID_input=info.input(n).ID; break;
        end
    end
    N=length(info.output);
    disp("OUTPUTS...");
    for n=1:N
        disp(info.output(n));
        if info.output(n).Name == "Reproduktory (USB Advanced Audio Device) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "Reproduktory (2 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "Reproduktory (3 - USB Advanced Audio Device) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "Reproduktory (3 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "Reproduktory (4 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "Reproduktory (5 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "Reproduktory (6 - USB AUDIO DEVICE) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
        if info.output(n).Name == "1-2 (OCTA-CAPTURE) (Windows DirectSound)"
            ID_output=info.output(n).ID; break;
        end
    end
end
