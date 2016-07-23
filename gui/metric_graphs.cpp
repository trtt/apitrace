#include "metric_graphs.h"

MetricGraphs::MetricGraphs(MetricCallDataModel& model) : index(0), model(model) {
    filter = std::make_shared<TextureBufferData<GLuint>>(*model.calls().programData());
    for (auto& m : model.metrics()) {
        textureData.push_back(std::make_shared<TextureBufferData<GLfloat>>(*m.vector()));
        data.push_back(new BarGraphData(textureData.back(), filter));
    }
}

MetricGraphs::~MetricGraphs() {
    for (auto& g : data) {
        delete g;
    }
}

uint MetricGraphs::size() const {
    return data.size();
}

void MetricGraphs::next() {
    index++;
}

BarGraphData* MetricGraphs::graphdata() {
    // automatically increment index
    if (index < data.size()) return data[index];
    else return nullptr;
}

QString MetricGraphs::name() const {
    return model.metrics()[index].metric()->getName();
}
