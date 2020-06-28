#define CATCH_CONFIG_MAIN
#include "PostPicture.hpp"
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("Testing Picture Serialize", "[serialize/picture]")
{
    PostPictureWrapper pA, pB;
    auto& picA = pA.picture();
    auto& picB = pB.picture();

    picA.data = new char[5]{'\0', '\0', 'f', 'o', 'o'};
    picA.size = 5;
    picA.objectType = PictureObjectType::face;
    picA.cameraId = 100;
    picA.objectId = 1;
    picA.width = 1;
    picA.height = 1;
    picA.timestamp = 10000;

    nlohmann::json json(picA);
    json.get_to(picB);

    CHECK(picA == picB);
}
