#include "metric_data_model.hpp"
#include "bargraph.h"

class MetricGraphs : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint size READ size)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(BarGraphData* graphdata READ graphdata)

public:
    MetricGraphs(MetricCallDataModel& model);
    ~MetricGraphs();

    unsigned size() const;
    QString name() const;
    BarGraphData* graphdata();
    Q_INVOKABLE void next();

private:
    unsigned index;
    MetricCallDataModel& model;
    std::shared_ptr<TextureBufferData<GLuint>> filter;
    std::vector<std::shared_ptr<TextureBufferData<GLfloat>>> textureData;
    std::vector<BarGraphData*> data;
};
