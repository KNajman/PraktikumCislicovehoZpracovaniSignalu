function [outputArg1,outputArg2] = My_Extern_Callback(app, iD, nData, xData)
outputArg1 = 1;outputArg2 = 2;
    
    switch iD
        case 35000
            figure(1);
            plot(xData);
            figure(2);
            % AmplSpectr(xData, nData, 48000);
            AmplSpectr(xData, 480, 48000);
        case 15000
            figure(1);
            title("Doba: " + xData + " tiků, tj. " + xData/120 + " us.");
        case 10001
                    app.D10001Label.Text = string(xData) + newline + app.D10001Label.Text;
        otherwise
    end

end