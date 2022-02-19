/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <Eigen/Core>
#include <iostream>
#include <vector>

using nanogui::Matrix3d;
using nanogui::MatrixXd;
using nanogui::Vector2d;
using nanogui::Vector2i;

#if 0
class Vector2i {
public:
	Vector2i(int xP, int yP) : xPos(xP), yPos(xP) {}
	int x() { return xPos; }
	int y() { return yPos; }
protected:
	int xPos, yPos;
};
#endif

#define TO_STRING(ID) #ID

class Widget {
  public:
    Vector2i position() { return mPos; }
    void setPosition(Vector2i p) { mPos = p; }
    Vector2i size() { return mSize; }
    void setSize(Vector2i s) { mSize = s; }

  protected:
    Vector2i mPos;
    Vector2i mSize;
};

/*
Handles have a mode and a position. Each object has 9 handles but since only one object's handles are
active at a time we can use a singleton ObjectHandles to hold the handles and initialise it based on
the position and size of an object.

*/
class Handle {
  public:
    enum Mode {
        NONE,
        POSITION,
        RESIZE_TL,
        RESIZE_T,
        RESIZE_TR,
        RESIZE_R,
        RESIZE_BL,
        RESIZE_L,
        RESIZE_BR,
        RESIZE_B
    };

    static Handle create(Mode which, Vector2i pos, Vector2i size) {
        Handle result;
        result.setMode(which);
        switch (which) {
        case NONE:
            break;
        case POSITION:
            result.setPosition(Vector2i(pos.x() + size.x() / 2, pos.y() + size.y() / 2));
            break;
        case RESIZE_TL:
            result.setPosition(Vector2i(pos.x(), pos.y()));
            break;
        case RESIZE_T:
            result.setPosition(Vector2i(pos.x() + size.x() / 2, pos.y()));
            break;
        case RESIZE_TR:
            result.setPosition(Vector2i(pos.x() + size.x(), pos.y()));
            break;
        case RESIZE_R:
            result.setPosition(Vector2i(pos.x() + size.x(), pos.y() + size.y() / 2));
            break;
        case RESIZE_BL:
            result.setPosition(Vector2i(pos.x(), pos.y() + size.y()));
            break;
        case RESIZE_L:
            result.setPosition(Vector2i(pos.x(), pos.y() + size.y() / 2));
            break;
        case RESIZE_BR:
            result.setPosition(Vector2i(pos.x() + size.x(), pos.y() + size.y()));
            break;
        case RESIZE_B:
            result.setPosition(Vector2i(pos.x() + size.x() / 2, pos.y() + size.y()));
            break;
        }
        return result;
    }

    std::ostream &operator<<(std::ostream &out) const { return out; }

    Handle() : mMode(NONE) {}

    void setPosition(Vector2i newpos) { pos = newpos; }
    Vector2i position() const { return pos; }

    void setMode(Mode newmode) { mMode = newmode; }
    Mode mode() { return mMode; }

    Handle closest(Vector2i pt) {
        Handle result;

        return result;
    }

  protected:
    Mode mMode;
    Vector2i pos;
};

std::ostream &operator<<(std::ostream &out, Handle::Mode m) {
    switch (m) {
    case Handle::NONE:
        out << TO_STRING(NONE);
        break;
    case Handle::POSITION:
        out << TO_STRING(POSITION);
        break;
    case Handle::RESIZE_TL:
        out << TO_STRING(RESIZE_TL);
        break;
    case Handle::RESIZE_T:
        out << TO_STRING(RESIZE_T);
        break;
    case Handle::RESIZE_TR:
        out << TO_STRING(RESIZE_TR);
        break;
    case Handle::RESIZE_R:
        out << TO_STRING(RESIZE_R);
        break;
    case Handle::RESIZE_BL:
        out << TO_STRING(RESIZE_BL);
        break;
    case Handle::RESIZE_L:
        out << TO_STRING(RESIZE_L);
        break;
    case Handle::RESIZE_BR:
        out << TO_STRING(RESIZE_BR);
        break;
    case Handle::RESIZE_B:
        out << TO_STRING(RESIZE_B);
        break;
    }
    return out;
}

//std::vector<Handle>handles(9);
Handle::Mode all_handles[] = {Handle::POSITION,  Handle::RESIZE_TL, Handle::RESIZE_T,
                              Handle::RESIZE_TR, Handle::RESIZE_R,  Handle::RESIZE_BL,
                              Handle::RESIZE_L,  Handle::RESIZE_BR, Handle::RESIZE_B};

using namespace std;
int main(int argc, char *argv[]) {
    const int ROWS = 5;
    Vector2i pos;
    nanogui::MatrixXd pts(ROWS, 2);
    pts = nanogui::MatrixXd::Random(ROWS, 2);
    std::cout << "pts:\n" << pts << "\n";
    Vector2d pt;
    pt(0, 0) = 0.5;
    pt(1, 0) = 0.5;
    std::cout << "rowwise sum:\n" << pts.rowwise().sum() << "\n";
    // find the distances to each pt

    std::cout << "by hand:\n"
              << ((pts(0, 0) - 0.5) * (pts(0, 0) - 0.5) + (pts(0, 1) - 0.5) * (pts(0, 1) - 0.5))
              << "\n";
    std::cout << "by hand:\n"
              << ((pts(1, 0) - 0.5) * (pts(1, 0) - 0.5) + (pts(1, 1) - 0.5) * (pts(1, 1) - 0.5))
              << "\n";

    nanogui::VectorXd dist = (pts.rowwise() -= pt.transpose()).rowwise().squaredNorm();
    std::cout << "distances:\n" << dist << "\n";
    double min = dist.x();
    int idx = 0;
    for (int i = 1; i < ROWS; ++i) {
        if (dist.row(i).x() < min) {
            min = dist.row(i).x();
            idx = i;
        }
    }
    std::cout << "smallest distance: " << min << "\n";

    Widget w;
    w.setPosition(Vector2i(100, 100));
    w.setSize(Vector2i(50, 30));
    MatrixXd z(9, 2);
    std::vector<Handle> handles(9);
    for (int i = 0; i < 9; ++i) {
        Handle h = Handle::create(all_handles[i], w.position(), w.size());
        z(i, 0) = h.position().x();
        z(i, 1) = h.position().y();
        handles[i] = h;
    }
    pt = Vector2d(124, 114);
    nanogui::VectorXd distances = (z.rowwise() - pt.transpose()).rowwise().squaredNorm();
    std::cout << "distances\n" << distances << "\n";
    min = distances.row(0).x();
    idx = 0;
    for (int i = 1; i < 9; ++i) {
        if (distances.row(i).x() < min) {
            min = distances.row(i).x();
            idx = i;
        }
    }
    if (min >= 30)
        std::cout << "no handle\n";
    else
        std::cout << "smallest distance: " << min << " " << handles[idx].mode() << "\n";

    /*	
	nanogui::Matrix3d m = Matrix3d::Random();
	cout << "Here is the matrix m:" << endl << m << endl;
	cout << "Here is the sum of each row:" << endl << m.rowwise().sum() << endl;
*/
}