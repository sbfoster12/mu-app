#include <data_products/nalu/NaluWaveform.hh>

int main() {

    dataProducts::NaluWaveform waveform;

    std::cout << "NaluWaveform for channel " << waveform.channel_num << " created." << std::endl;

    return 0;
}
