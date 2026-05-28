import unittest
from pathlib import Path

from scripts import check_public_header_contracts


class PublicHeaderContractsTest(unittest.TestCase):
    maxDiff = None

    def test_net_callback_typedefs_have_doxygen_comments(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_net_callback_typedefs(Path(".")),
        )

    def test_net_callback_typedefs_document_threading_contracts(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_net_callback_threading_contracts(Path(".")),
        )

    def test_net_zero_timeout_fields_document_zero_semantics(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_net_zero_timeout_field_contracts(Path(".")),
        )

    def test_tracked_enum_values_have_doxygen_comments(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_tracked_enum_values(Path(".")),
        )

    def test_tracked_public_lifecycle_methods_have_doxygen_comments(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_tracked_lifecycle_methods(Path(".")),
        )

    def test_public_headers_use_ascii_dash_punctuation(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_public_header_dash_punctuation(Path(".")),
        )

    def test_move_only_classes_document_ownership_contracts(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_move_only_ownership_contracts(Path(".")),
        )

    def test_result_apis_document_error_contracts(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_result_error_contracts(Path(".")),
        )

    def test_status_apis_document_error_contracts(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_status_error_contracts(Path(".")),
        )

    def test_opaque_identifier_fields_document_lifecycle_contracts(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_opaque_identifier_contracts(Path(".")),
        )

    def test_screen_window_capture_documents_visible_pixel_semantics(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_screen_visible_window_contracts(Path(".")),
        )

    def test_screen_frame_documents_pixel_layout_contract(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_screen_frame_layout_contracts(Path(".")),
        )

    def test_media_frame_documents_data_layout_contract(self):
        self.assertEqual(
            [],
            check_public_header_contracts.check_media_frame_layout_contracts(Path(".")),
        )


if __name__ == "__main__":
    unittest.main()
