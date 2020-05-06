#include <QApplication>
#include <QDebug>
#include <ffmpeg_demuxing_decode.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ffmpegDemuxingDecode ff1;
    std::string name = "D:/DataSet/video-h265.mkv";
    ff1.set_srcFilename(name);
    int ret = ff1.init();
    if (ret != 0) {
        fprintf(stderr, "init failed");
    }

    while(1) {
        int nn = 0;
        cv::Mat frame = ff1.demux_decode_a_frame(nn);
        if (!frame.empty()) {
            cv::imshow("frame", frame);
            cv::waitKey(10);
        } else {
            cv::destroyAllWindows();
            break;
        }
    }
    ff1.release();

    qDebug() << "hello world! ff1" << endl;

    return a.exec();
}
