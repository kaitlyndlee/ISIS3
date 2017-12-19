#include "ControlPointV0001.h"

#include <QString>

#include "ControlPointV0001.h"
#include "Distance.h"
#include "Pvl.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace std;

namespace Isis {

  /**
   * Create a ControlPointV0001 object from a protobuf version 1 control point message.
   *
   * @param pointData The protobuf message from a control net file.
   */
  ControlPointV0001::ControlPointV0001(
          QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> pointData)
   : m_pointData(pointData) {

   }


  /**
   * Create a ControlPointV0001 object from a version 1 control point Pvl object
   *
   * @param pointObject The control point and its measures in a Pvl object
   * @param targetName The name of the taret used to get the body radii when converting from
   *                   lat/lon to x/y/z.
   */
  ControlPointV0001::ControlPointV0001(Pvl &pointObject, const QString targetName)
   : m_pointData(new ControlNetFileProtoV0001_PBControlPoint) {

    // Clean up the Pvl control point
    // Anything that doesn't have a value is removed
    for (int cpKeyIndex = 0; cpKeyIndex < pointObject.keywords(); cpKeyIndex ++) {
      if (pointObject[cpKeyIndex][0] == "") {
        pointObject.deleteKeyword(cpKeyIndex);
      }
    }

    // Copy over POD values
    copy(pointObject, "PointId",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_id);
    copy(pointObject, "ChooserName",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_choosername);
    copy(pointObject, "DateTime",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_datetime);
    copy(pointObject, "AprioriLatLonSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_apriorisurfpointsourcefile);
    copy(pointObject, "AprioriRadiusSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_aprioriradiussourcefile);
    copy(pointObject, "JigsawRejected",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_jigsawrejected);
    copy(pointObject, "EditLock",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_editlock);
    copy(pointObject, "Ignore",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_ignore);
    copy(pointObject, "LatitudeConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_latitudeconstrained);
    copy(pointObject, "LongitudeConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_longitudeconstrained);
    copy(pointObject, "RadiusConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_radiusconstrained);

    // Copy over the adjusted surface point
    if ( pointObject.hasKeyword("Latitude")
         && pointObject.hasKeyword("Longitude")
         && pointObject.hasKeyword("Radius") ) {
      SurfacePoint adjustedPoint(
          Latitude(toDouble(pointObject["Latitude"][0]), Angle::Degrees),
          Longitude(toDouble(pointObject["Longitude"][0]), Angle::Degrees),
          Distance(toDouble(pointObject["Radius"][0]), Distance::Meters));

      m_pointData->set_adjustedx( adjustedPoint.GetX().meters() );
      m_pointData->set_adjustedy( adjustedPoint.GetY().meters() );
      m_pointData->set_adjustedz( adjustedPoint.GetZ().meters() );
    }
    else if ( pointObject.hasKeyword("X")
              && pointObject.hasKeyword("Y")
              && pointObject.hasKeyword("Z") ) {
      m_pointData->set_adjustedx( pointObject["Latitude"][0] );
      m_pointData->set_adjustedy( pointObject["Longitude"][0] );
      m_pointData->set_adjustedz( pointObject["Radius"][0] );
    }
    else {
      QString msg = "Unable to find adjusted surface point values for the control point.";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }

    // copy over the apriori surface point
    if ( pointObject.hasKeyword("AprioriLatitude")
         && pointObject.hasKeyword("AprioriLongitude")
         && pointObject.hasKeyword("AprioriRadius") ) {
      SurfacePoint aprioriPoint(
          Latitude(toDouble(pointObject["AprioriLatitude"][0]), Angle::Degrees),
          Longitude(toDouble(pointObject["AprioriLongitude"][0]), Angle::Degrees),
          Distance(toDouble(pointObject["AprioriRadius"][0]), Distance::Meters));

      m_pointData->set_apriorix( aprioriPoint.GetX().meters() );
      m_pointData->set_aprioriy( aprioriPoint.GetY().meters() );
      m_pointData->set_aprioriz( aprioriPoint.GetZ().meters() );
    }
    // If the apriori values are missing, copy them from the adjusted.
    else if ( m_pointData->has_adjustedx()
              && m_pointData->has_adjustedy()
              && m_pointData->has_adjustedz() ) {
      m_pointData->set_apriorix( m_pointData->adjustedx() );
      m_pointData->set_aprioriy( m_pointData->adjustedy() );
      m_pointData->set_aprioriz( m_pointData->adjustedz() );
    }
    else {
      QString msg = "Unable to find apriori surface point values for the control point.";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }

    // Ground points were previously flagged by the Held keyword being true.
    if (pointObject.hasKeyword("Held") && pointObject["Held"][0] == "True") {
      m_pointData->set_type(ControlNetFileProtoV0001_PBControlPoint::Ground);
    }
    else if (pointObject["PointType"][0] == "Tie") {
      m_pointData->set_type(ControlNetFileProtoV0001_PBControlPoint::Tie);
    }
    else {
      QString msg = "Invalid ControlPoint type [" + pointObject["PointType"][0] + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (pointObject.hasKeyword("AprioriLatLonSource")) {
      QString source = pointObject["AprioriLatLonSource"][0];

      if (source == "None") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::None);
      }
      else if (source == "User") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::User);
      }
      else if (source == "AverageOfMeasures") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::AverageOfMeasures);
      }
      else if (source == "Reference") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::Reference);
      }
      else if (source == "Basemap") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::Basemap);
      }
      else if (source == "BundleSolution") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::BundleSolution);
      }
      else {
        QString msg = "Invalid AprioriLatLonSource [" + source + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    if (pointObject.hasKeyword("AprioriRadiusSource")) {
      QString source = pointObject["AprioriRadiusSource"][0];

      if (source == "None") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::None);
      }
      else if (source == "User") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::User);
      }
      else if (source == "AverageOfMeasures") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::AverageOfMeasures);
      }
      else if (source == "Ellipsoid") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::Ellipsoid);
      }
      else if (source == "DEM") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::DEM);
      }
      else if (source == "BundleSolution") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::BundleSolution);
      }
      else {
        QString msg = "Invalid AprioriRadiusSource, [" + source + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Copy the covariance matrices
    // Sometimes they are not stored in version 1 Pvls so we compute them from a combination
    // of the surface point sigmas and the target radii.

    // We have to do this because Target::radiiGroup calls NAIF routines,
    // but doesn't check for errors.
    NaifStatus::CheckErrors();
    PvlGroup radii;
    try {
     radii = Target::radiiGroup(targetName);
    }
    catch (IException &e) {
     try {
       NaifStatus::CheckErrors();
     }
     catch (IException &) {
       // pass to the outer catch
     }
     QString msg = "Unable to get target body radii for [" + targetName
                   + "] when calculating covariance matrix.";
     throw IException(e, IException::Io, msg, _FILEINFO_);
    }
    Distance equatorialRadius(radii["EquatorialRadius"], Distance::Meters);
    Distance polarRadius(radii["PolarRadius"], Distance::Meters);

    if ( pointObject.hasKeyword("ApostCovarianceMatrix") ) {
      PvlKeyword &matrix = pointObject["ApostCovarianceMatrix"];

      m_pointData->add_aprioricovar(toDouble(matrix[0]));
      m_pointData->add_aprioricovar(toDouble(matrix[1]));
      m_pointData->add_aprioricovar(toDouble(matrix[2]));
      m_pointData->add_aprioricovar(toDouble(matrix[3]));
      m_pointData->add_aprioricovar(toDouble(matrix[4]));
      m_pointData->add_aprioricovar(toDouble(matrix[5]));

      m_pointData->set_latitudeconstrained(true);
      m_pointData->set_longitudeconstrained(true);
      m_pointData->set_radiusconstrained(true);
    }
    else if ( pointObject.hasKeyword("AprioriSigmaLatitude")
              || pointObject.hasKeyword("AprioriSigmaLongitude")
              || pointObject.hasKeyword("AprioriSigmaRadius") ) {
      // There may be missing or negative apriori sigmas so default to 10,000
      double sigmaLat = 10000.0;
      double sigmaLon = 10000.0;
      double sigmaRad = 10000.0;

      if ( pointObject.hasKeyword("AprioriSigmaLatitude") ) {
        if (toDouble(pointObject["AprioriSigmaLatitude"][0]) > 0
            && toDouble(pointObject["AprioriSigmaLatitude"][0]) < sigmaLat) {
          sigmaLat = toDouble(pointObject["AprioriSigmaLatitude"][0]);
        }
        m_pointData->set_latitudeconstrained(true);
      }

      if ( pointObject.hasKeyword("AprioriSigmaLongitude") ) {
        if (toDouble(pointObject["AprioriSigmaLongitude"][0]) > 0
            && toDouble(pointObject["AprioriSigmaLongitude"][0]) < sigmaLon) {
          sigmaLon = toDouble(pointObject["AprioriSigmaLongitude"][0]);
        }
        m_pointData->set_longitudeconstrained(true);
      }

      if ( pointObject.hasKeyword("AprioriSigmaRadius") ) {
        if (toDouble(pointObject["AprioriSigmaRadius"][0]) > 0
            && toDouble(pointObject["AprioriSigmaRadius"][0]) < sigmaRad) {
          sigmaRad = toDouble(pointObject["AprioriSigmaRadius"][0]);
        }
        m_pointData->set_radiusconstrained(true);
      }

      SurfacePoint aprioriPoint;
      aprioriPoint.SetRadii(equatorialRadius, equatorialRadius, polarRadius);
      aprioriPoint.SetRectangular( Displacement(m_pointData->apriorix(), Displacement::Meters),
                                   Displacement(m_pointData->aprioriy(), Displacement::Meters),
                                   Displacement(m_pointData->aprioriz(), Displacement::Meters) );
      aprioriPoint.SetSphericalSigmasDistance( Distance(sigmaLat, Distance::Meters),
                                               Distance(sigmaLon, Distance::Meters),
                                               Distance(sigmaRad, Distance::Meters) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(0, 0) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(0, 1) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(0, 2) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(1, 1) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(1, 2) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(2, 2) );
    }

    if ( pointObject.hasKeyword("AdjustedSigmaLatitude")
         || pointObject.hasKeyword("AdjustedSigmaLongitude")
         || pointObject.hasKeyword("AdjustedSigmaRadius") ) {
      // There may be missing or negative adjusted sigmas so default to 10,000
      double sigmaLat = 10000.0;
      double sigmaLon = 10000.0;
      double sigmaRad = 10000.0;

      if ( pointObject.hasKeyword("AdjustedSigmaLatitude") ) {
        if (toDouble(pointObject["AdjustedSigmaLatitude"][0]) > 0
            && toDouble(pointObject["AdjustedSigmaLatitude"][0]) < sigmaLat) {
          sigmaLat = toDouble(pointObject["AdjustedSigmaLatitude"][0]);
        }
      }

      if ( pointObject.hasKeyword("AdjustedSigmaLongitude") ) {
        if (toDouble(pointObject["AdjustedSigmaLongitude"][0]) > 0
            && toDouble(pointObject["AdjustedSigmaLongitude"][0]) < sigmaLon) {
          sigmaLon = toDouble(pointObject["AdjustedSigmaLongitude"][0]);
        }
      }

      if ( pointObject.hasKeyword("AdjustedSigmaRadius") ) {
        if (toDouble(pointObject["AdjustedSigmaRadius"][0]) > 0
            && toDouble(pointObject["AdjustedSigmaRadius"][0]) < sigmaRad) {
          sigmaRad = toDouble(pointObject["AdjustedSigmaRadius"][0]);
        }
      }

      SurfacePoint adjustedPoint;
      adjustedPoint.SetRadii(equatorialRadius, equatorialRadius, polarRadius);
      adjustedPoint.SetRectangular( Displacement(m_pointData->adjustedx(), Displacement::Meters),
                                    Displacement(m_pointData->adjustedy(), Displacement::Meters),
                                    Displacement(m_pointData->adjustedz(), Displacement::Meters) );
      adjustedPoint.SetSphericalSigmasDistance( Distance(sigmaLat, Distance::Meters),
                                                Distance(sigmaLon, Distance::Meters),
                                                Distance(sigmaRad, Distance::Meters) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(0, 0) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(0, 1) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(0, 2) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(1, 1) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(1, 2) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(2, 2) );
    }

    //  Process Measures
    for (int groupIndex = 0; groupIndex < pointObject.groups(); groupIndex ++) {
      PvlGroup &group = pointObject.group(groupIndex);
      ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure measure;

      // Copy strings, booleans, and doubles
      copy(group, "SerialNumber",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_serialnumber);
      copy(group, "ChooserName",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_choosername);
      copy(group, "DateTime",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_datetime);
      copy(group, "Diameter",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_diameter);
      copy(group, "EditLock",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_editlock);
      copy(group, "Ignore",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_ignore);
      copy(group, "JigsawRejected",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_jigsawrejected);
      copy(group, "AprioriSample",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_apriorisample);
      copy(group, "AprioriLine",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_aprioriline);
      copy(group, "SampleSigma",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_samplesigma);
      copy(group, "LineSigma",
           measure, &ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::set_linesigma);

      // The sample, line, sample residual, and line residual are nested in another structure
      // inside the measure, so they cannot be copied with the conenience methods.
      if (group.hasKeyword("Sample")) {
        // The sample may not be a numeric value
        // in this case set it to 0 and ignore the measure
        double value
        try {
          value = toDouble(group["Sample"][0]);
        }
        catch (...) {
          value = 0;
          m_pointData->set_ignore(true);
        }
        measure.measurement().set_sample(value);
        group.deleteKeyword("Sample");
      }
      if (group.hasKeyword("Line")) {
        // The line may not be a numeric value
        // in this case set it to 0 and ignore the measure
        double value
        try {
          value = toDouble(group["Line"][0]);
        }
        catch (...) {
          value = 0;
          m_pointData->set_ignore(true);
        }
        measure.measurement().set_line(value);
        group.deleteKeyword("Line");
      }
      if (group.hasKeyword("ErrorSample")) {
        double value = toDouble(group["ErrorSample"][0]);
        measure.measurement().set_sampleresidual(value);
        group.deleteKeyword("ErrorSample");
      }
      if (group.hasKeyword("ErrorLine")) {
        double value = toDouble(group["ErrorLine"][0]);
        measure.measurement().set_lineresidual(value);
        group.deleteKeyword("ErrorLine");
      }

      if (group.hasKeyword("Reference")) {
        if (group["Reference"][0].toLower() == "true") {
          m_pointData->set_referenceindex(groupIndex);
        }
        group.deleteKeyword("Reference");
      }

      // Copy the measure type
      if (group.hasKeyword("MeasureType")) {
        QString type = group["MeasureType"][0].toLower();
        if (type == "estimated"
            || type == "unmeasured") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::Candidate);
        }
        else if (type == "manual") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::Manual);
        }
        else if (type == "automatic"
                 || type == "validatedmanual"
                 || type == "automaticpixel") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::RegisteredPixel);
        }
        else if (type == "validatedautomatic"
                 || type == "automaticsubpixel") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::RegisteredSubPixel);
        }
        else {
          throw IException(IException::Io,
                           "Unknown measure type [" + type + "]",
                           _FILEINFO_);
        }
        group.deleteKeyword("MeasureType");
      }

      // Clean up the remaining keywords
      for (int cmKeyIndex = 0; cmKeyIndex < group.keywords(); cmKeyIndex ++) {
        if (group[cmKeyIndex][0] == ""
            || group[cmKeyIndex].name() == "ZScore"
            || group[cmKeyIndex].name() == "ErrorMagnitude") {
          group.deleteKeyword(cmKeyIndex);
        }
      }

      for (int key = 0; key < group.keywords(); key++) {
        ControlMeasureLogData interpreter(group[key]);
        if (!interpreter.IsValid()) {
          QString msg = "Unhandled or duplicate keywords in control measure ["
                        + group[key].name() + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        else {
          *measure.add_log() = interpreter.ToProtocolBuffer();
        }
      }

      *m_pointData->add_measures() = measure;
    }

    if (!m_pointData->IsInitialized()) {
      QString msg = "There is missing required information in the control "
                    "points or measures";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }


  /**
   * Access the protobuf control point data.
   *
   * @return @b QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> A pointer to the internal
   *                                                                    control point data.
   */
  QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> ControlPointV0001::pointData() {
      return m_pointData;
  }


  /**
   * This convenience method takes a boolean value from a PvlKeyword and copies it into a version 1
   * protobuf field.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param point[out] The version 1 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> point,
                               void (ControlNetFileProtoV0001_PBControlPoint::*setter)(bool)) {

    if (!container.hasKeyword(keyName))
      return;

    QString value = container[keyName][0];
    container.deleteKeyword(keyName);
    value = value.toLower();

    if (value == "true" || value == "yes")
      (point->*setter)(true);
  }


  /**
   * This convenience method takes a double value from a PvlKeyword and copies it into a version 1
   * protobuf field.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param point[out] The version 1 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> &point,
                               void (ControlNetFileProtoV0001_PBControlPoint::*setter)(double)) {

    if (!container.hasKeyword(keyName))
      return;

    double value = toDouble(container[keyName][0]);
    container.deleteKeyword(keyName);
    (point->*setter)(value);
  }


  /**
   * This convenience method takes a string value from a PvlKeyword and copies it into a version 1
   * protobuf field.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param point[out] The version 1 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> &point,
                               void (ControlNetFileProtoV0001_PBControlPoint::*setter)(const std::string&)) {

    if (!container.hasKeyword(keyName))
      return;

    QString value = container[keyName][0];
    container.deleteKeyword(keyName);
    (point->*setter)(value);
  }


  /**
   * This convenience method takes a boolean value from a PvlKeyword and copies it into a version 1
   * protobuf field.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 1 protobuf representation of the control measure that the
   *                     value will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::*setter)(bool)) {

    if (!container.hasKeyword(keyName))
      return;

    QString value = container[keyName][0];
    container.deleteKeyword(keyName);
    value = value.toLower();

    if (value == "true" || value == "yes")
      (measure.*setter)(true);
  }


  /**
   * This convenience method takes a double value from a PvlKeyword and copies it into a version 1
   * protobuf field.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 1 protobuf representation of the control measure that the
   *                     value will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::*setter)(double)) {

    if (!container.hasKeyword(keyName))
      return;

    double value = toDouble(container[keyName][0]);
    container.deleteKeyword(keyName);
    (measure.*setter)(value);
  }


  /**
   * This convenience method takes a string value from a PvlKeyword and copies it into a version 1
   * protobuf field.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 1 protobuf representation of the control measure that the
   *                     value will be into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::*setter)
                                      (const std::string &)) {

    if (!container.hasKeyword(keyName))
      return;

    QString value = container[keyName][0];
    container.deleteKeyword(keyName);
    (measure.*set)(value);
  }
}