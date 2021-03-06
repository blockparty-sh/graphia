#ifndef CONTAINER_RANDOMSAMPLE_H
#define CONTAINER_RANDOMSAMPLE_H

#include <random>

namespace u
{
template<typename T, template<typename, typename...> typename C, typename... Args>
C<T, Args...> randomSample(const C<T, Args...>& container, size_t numSamples)
{
    if(numSamples > container.size())
        return container;

    auto sample = container;

    std::random_device rd;
    std::default_random_engine dre(rd());

    for(size_t i = 0; i < numSamples; i++)
    {
        int high = static_cast<int>(sample.size() - i) - 1;
        std::uniform_int_distribution uid(0, high);
        std::swap(sample[i], sample[i + uid(dre)]);
    }

    sample.resize(numSamples);

    return sample;
}
} // namespace u

#endif // CONTAINER_RANDOMSAMPLE_H
