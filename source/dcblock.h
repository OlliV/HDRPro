namespace MyVst {
using namespace Steinberg::Vst;

template <typename SampleType>
class DCBlock {
private:
    const SampleType a = 0.95;
    SampleType wn;
public:
    DCblock()
    {
        wn = 0.0;
    }

    void reset(float sampleRate)
    {
        wn = 0.0;
    }

    SampleType process(SampleType xn)
    {
        const SampleType wn_prev = wn;

        wn = xn + a * wn_prev;
        return wn - wn_prev;
    }
};
}
