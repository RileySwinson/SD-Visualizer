#include "geometryRenderer.h"

void boxEdges(
    const Eigen::Vector3f& center, const Eigen::Vector3f& size,
    std::vector<Eigen::Vector3f>& out
) {
    Eigen::Vector3f half = size * 0.5f;
    Eigen::Vector3f lo   = center - half;
    Eigen::Vector3f hi   = center + half;

    auto edge = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
        out.push_back({ x0, y0, z0 });
        out.push_back({ x1, y1, z1 });
    };

    edge(lo.x(), lo.y(), lo.z(), hi.x(), lo.y(), lo.z());
    edge(hi.x(), lo.y(), lo.z(), hi.x(), hi.y(), lo.z());
    edge(hi.x(), hi.y(), lo.z(), lo.x(), hi.y(), lo.z());
    edge(lo.x(), hi.y(), lo.z(), lo.x(), lo.y(), lo.z());
    edge(lo.x(), lo.y(), hi.z(), hi.x(), lo.y(), hi.z());
    edge(hi.x(), lo.y(), hi.z(), hi.x(), hi.y(), hi.z());
    edge(hi.x(), hi.y(), hi.z(), lo.x(), hi.y(), hi.z());
    edge(lo.x(), hi.y(), hi.z(), lo.x(), lo.y(), hi.z());
    edge(lo.x(), lo.y(), lo.z(), lo.x(), lo.y(), hi.z());
    edge(hi.x(), lo.y(), lo.z(), hi.x(), lo.y(), hi.z());
    edge(hi.x(), hi.y(), lo.z(), hi.x(), hi.y(), hi.z());
    edge(lo.x(), hi.y(), lo.z(), lo.x(), hi.y(), hi.z());
}

void boxEdgesMinMax(
    const Eigen::Vector3f& lo, const Eigen::Vector3f& hi,
    std::vector<Eigen::Vector3f>& out
) {
    boxEdges((lo + hi) * 0.5f, hi - lo, out);
}

void drawLines(
    GLuint lineProg, GLuint lineVAO, GLuint lineVBO,
    const std::vector<Eigen::Vector3f>& points,
    const Eigen::Matrix4f& mvp,
    std::array<float, 4> color, float lineWidth
) {
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 points.size() * sizeof(Eigen::Vector3f),
                 points.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Eigen::Vector3f), 0);
    glEnableVertexAttribArray(0);

    glUseProgram(lineProg);
    glUniformMatrix4fv(glGetUniformLocation(lineProg, "uMVP"), 1, GL_FALSE, mvp.data());
    glUniform4f(glGetUniformLocation(lineProg, "uColor"),
                color[0], color[1], color[2], color[3]);

    glLineWidth(lineWidth);
    glDrawArrays(GL_LINES, 0, (int)points.size());
    glBindVertexArray(0);
}

void drawBox(
    GLuint lineProg, GLuint lineVAO, GLuint lineVBO,
    const Eigen::Vector3f& center, const Eigen::Vector3f& size,
    const Eigen::Matrix4f& mvp,
    std::array<float, 4> color, float lineWidth
) {
    std::vector<Eigen::Vector3f> edges;
    boxEdges(center, size, edges);
    drawLines(lineProg, lineVAO, lineVBO, edges, mvp, color, lineWidth);
}
