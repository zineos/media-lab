/*
* Copyright (c) 2019, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <string>

#include "DataPacket.h"
#include "ConnectorRR.h"
#include "CropThreadBlock.h"
#include "DecodeThreadBlock.h"
#include "InferenceThreadBlock.h"
#include "Statistics.h"


const std::string input_file = "/home/hefan/workspace/VA/video_analytics_Intel_GPU/test_content/video/0.h264";
const std::string input_file1 = "/home/hefan/workspace/VA/video_analytics_Intel_GPU/test_content/video/1.h264";

int main(int argc, char *argv[])
{
    int channel_num = 2;
    int inference_num = 1;
    int crop_num = 1;
    DecodeThreadBlock **decodeBlocks = new DecodeThreadBlock *[channel_num];
    InferenceThreadBlock **inferBlocks = new InferenceThreadBlock *[inference_num];
    CropThreadBlock **cropBlocks = new CropThreadBlock *[channel_num];
    VAFilePin **filePins = new VAFilePin *[channel_num];
    VASinkPin **sinks = new VASinkPin *[crop_num];

    VAConnectorRR *c1 = new VAConnectorRR(channel_num, inference_num, 10);
    VAConnectorRR *c2 = new VAConnectorRR(inference_num, crop_num, 10);

    for (int i = 0; i < channel_num; i++)
    {
        DecodeThreadBlock *dec = decodeBlocks[i] = new DecodeThreadBlock(i);
        VAFilePin *pin = nullptr;
        if (i == 0)
            pin = filePins[i] = new VAFilePin(input_file.c_str());
        else
            pin = filePins[i] = new VAFilePin(input_file1.c_str());
        dec->ConnectInput(pin);
        dec->ConnectOutput(c1->NewInputPin());
        dec->SetDecodeOutputRef(1);
        dec->SetVPOutputRef(1);
        dec->SetVPRatio(1);
        dec->SetVPOutResolution(300, 300);
        dec->Prepare();
    }
    printf("After decoder prepare\n");

    for (int i = 0; i < inference_num; i++)
    {
        InferenceThreadBlock *infer = inferBlocks[i] = new InferenceThreadBlock(i, MOBILENET_SSD_U8);
        infer->ConnectInput(c1->NewOutputPin());
        infer->ConnectOutput(c2->NewInputPin());
        infer->SetAsyncDepth(1);
        infer->SetBatchNum(1);
        infer->SetDevice("GPU");
        infer->SetModelFile("../../models/mobilenet-ssd.xml", "../../models/mobilenet-ssd.bin");
        infer->Prepare();
    }
    printf("After inference prepare\n");

    for (int i = 0; i < crop_num; i++)
    {
        CropThreadBlock *crop = cropBlocks[i] = new CropThreadBlock(i);
        VASinkPin *sink = new VASinkPin();
        crop->ConnectInput(c2->NewOutputPin());
        crop->ConnectOutput(sink);
        crop->SetOutResolution(224, 224);
        crop->SetOutDump();
        crop->Prepare();
    }
    printf("After crop prepare\n");

    VAThreadBlock::RunAllThreads();

    Statistics::getInstance().ReportPeriodly(1.0);

    VAThreadBlock::StopAllThreads();

    for (int i = 0; i < channel_num; i++)
    {
        delete decodeBlocks[i];
    }
    for (int i = 0; i < inference_num; i++)
    {
        delete inferBlocks[i];
    }
    for (int i = 0; i < crop_num; i++)
    {
        delete cropBlocks[i];
    }

    return 0;
}
