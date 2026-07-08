function [outputArg1,outputArg2] = My_Extern_Button(app)
outputArg1 = 1;outputArg2 = 2;

    writeDataSTM32(app.s, 35000, 0, 0);
end